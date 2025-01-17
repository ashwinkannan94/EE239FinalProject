
/*Enhanced Magnetic Model (EMM) File processing program. The program can
accept the input parameters from a user-specified file. Then the EMM
program is called to compute the magnetic fields. The results are then
printed to the specified output file.

The Geomagnetism Library is used in this program. The program expects the
files EMM.COF and EGM9615.BIN to be in the same directory.

Note that the option for geocentric height (C) is not supported in this
version. The height entered is considered as height above mean sea level.

Manoj.C.Nair@Noaa.Gov
November 15, 2009

 *  Revision Number: $Revision: 1324 $
 *  Last changed by: $Author: awoods $
 *  Last changed on: $Date: 2015-05-13 16:04:23 -0600 (Wed, 13 May 2015) $




 */
/****************************************************************************/



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <math.h>               /* for gcc */
#include "GeomagnetismHeader.h"
#include "EGM9615.h"

#define NaN log(-1.0)
/* constants */
#define RECL 81

#define MAXINBUFF RECL+14

/** Max size of in buffer **/

#define MAXREAD MAXINBUFF-2
/** Max to read 2 less than total size (just to be safe) **/


#define PATH MAXREAD




/****************************************************************************/
/*                                                                          */
/*      Some variables used in this program                                 */
/*                                                                          */
/*    Name         Type                    Usage                            */
/* ------------------------------------------------------------------------ */

/*                                                                          */
/*   minalt     double array of MAXMOD  Minimum height of model.             */
/*                                                                          */
/*   altmin     double                  Minimum height of selected model.    */
/*                                                                          */
/*   altmax     double array of MAXMOD  Maximum height of model.             */
/*                                                                          */
/*   maxalt     double                  Maximum height of selected model.    */
/*                                                                          */
/*   sdate  Scalar double           start date inputted                      */
/*                                                                          */
/*   alt        Scalar double           altitude above WGS84 Ellipsoid       */
/*                                                                          */
/*   latitude   Scalar double           Latitude.                            */
/*                                                                          */
/*   longitude  Scalar double           Longitude.                           */
/*                                                                          */
/*   inbuff     Char a of MAXINBUF     Input buffer.                        */
/*                                                                          */
/*                                                                          */
/*   minyr      double                  Min year of all models               */
/*                                                                          */
/*   maxyr      double                  Max year of all models               */
/*                                                                          */
/*   yrmax      double array of MAXMOD  Max year of model.                   */
/*                                                                          */
/*   yrmin      double array of MAXMOD  Min year of model.                   */
/*                                                                          */

/****************************************************************************/


int main(int argv, char**argc)
{
#ifdef MAC
    ccommand(argv, argc);
#endif
    /*  WMM Variable declaration  */

    MAGtype_MagneticModel * MagneticModels[17], *TimedMagneticModel;
    MAGtype_Gradient Gradient;

    MAGtype_Ellipsoid Ellip;
    MAGtype_CoordSpherical CoordSpherical;
    MAGtype_CoordGeodetic CoordGeodetic;
    MAGtype_Date UserDate;
    MAGtype_GeoMagneticElements GeoMagneticElements;
    MAGtype_Geoid Geoid;
    char ans[20];
    char filename[] = "EMM2015.COF";
    char filenameSV[] = "EMM2015SV.COF";
    int NumTerms, epochs = 16, nMax = 0, LoadedEpoch = -1, Epoch;
    char VersionDate_Large[] = "$Date: 2015-05-13 16:04:23 -0600 (Wed, 13 May 2015) $";
    char VersionDate[12];

    /* Control variables */
    int again = 1;
    int decyears = 3;
    int units = 4;
    int decdeg = 3;
    int range = -1;
    int igdgc = 3;
    int isyear = -1;
    int ismonth = -1;
    int isday = -1;
    int ieyear = -1;
    int iemonth = -1;
    int ieday = -1;
    int ilat_deg = 200;
    int ilat_min = 200;
    int ilat_sec = 200;
    int ilon_deg = 200;
    int ilon_min = 200;
    int ilon_sec = 200;


    int coords_from_file = 0;
    int use_gradient = 0;
    int arg_err = 0;

    char inbuff[MAXINBUFF];

    char *begin;
    char *rest;
    char args[7][MAXREAD];
    int iarg;

    char coord_fname[PATH];
    char out_fname[PATH];
    FILE *coordfile, *outfile;
    int iline = 0;
    int read_flag;

    double minyr;
    double maxyr;
    double minalt;
    double maxalt;
    double alt = -999999;
    double sdate = -1;
    double step = -1;
    double edate = -1;
    double latitude = 200;
    double longitude = 200;



    /*  Subroutines used  */

    void print_result_file(FILE *outf, double d, double i, double h, double x, double y, double z, double f,
            double ddot, double idot, double hdot, double xdot, double ydot, double zdot, double fdot);
    void print_result_file_gradient(FILE *outf, MAGtype_GeoMagneticElements Output, MAGtype_Gradient OutputGradient);
    double degrees_to_decimal();
    double julday();
    int getshc();


    /* Initializations. */

    inbuff[MAXREAD + 1] = '\0'; /* Just to protect mem. */
    inbuff[MAXINBUFF - 1] = '\0'; /* Just to protect mem. */


    /* Memory allocation */
    strncpy(VersionDate, VersionDate_Large + 39, 11);
    VersionDate[11] = '\0';
    for(Epoch = 0; Epoch < epochs; Epoch++)
    {
        sprintf(filename, "EMM%d.COF", Epoch + 2000);
        sprintf(filenameSV, "EMM%dSV.COF", Epoch + 2000);
        if(Epoch == epochs-1)
            Epoch++;
        if(!MAG_robustReadMagneticModel_Large(filename, filenameSV, &MagneticModels[Epoch])) {
            printf("\n EMM%d.COF or EMM%dSV.COF not found.  Press enter to exit... \n ", Epoch+2000, Epoch+2000);
            fgets(ans, 20, stdin);
            return 1;
        }
    }
    nMax = MagneticModels[0]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2); 
    MagneticModels[epochs - 1] = MAG_AllocateModelMemory(NumTerms);
    MagneticModels[epochs - 1]->nMax = MagneticModels[0]->nMax;
    MagneticModels[epochs - 1]->nMaxSecVar = MagneticModels[0]->nMaxSecVar;
    MagneticModels[epochs - 1]->epoch = MagneticModels[0]->epoch + epochs - 1;
    MAG_AssignMagneticModelCoeffs(MagneticModels[epochs - 1], MagneticModels[epochs], MagneticModels[epochs - 1]->nMax, MagneticModels[epochs - 1]->nMaxSecVar);


    nMax = MagneticModels[epochs]->nMax;
    NumTerms = ((nMax + 1) * (nMax + 2) / 2);
    TimedMagneticModel = MAG_AllocateModelMemory(NumTerms); /* For storing the time modified WMM Model parameters */
    if(MagneticModels[0] == NULL || TimedMagneticModel == NULL)
    {
        MAG_Error(2);
    }

    MAG_SetDefaults(&Ellip, &Geoid); /* Set default values and constants */
    /* Check for Geographic Poles */
    //MAG_InitializeGeoid(&Geoid);    /* Read the Geoid file, Deprecated */
    /* Set EGM96 Geoid parameters */
    Geoid.GeoidHeightBuffer = GeoidHeightBuffer;
    Geoid.Geoid_Initialized = 1;
    /* Set EGM96 Geoid parameters END */
    maxyr = MagneticModels[epochs - 1]->epoch + 5.0;
    minyr = MagneticModels[0]->epoch;

    for(iarg = 0; iarg < argv; iarg++)
        if(argc[iarg] != NULL)
            strncpy(args[iarg], argc[iarg], MAXREAD);

    //printing out version number and header
    printf("\n\n EMM_FileProc File processing program %s ", VersionDate);

    if(argv == 1 || ((argv == 2) && (*(args[1]) == 'h')))
    {
        printf("\n\nEnhanced World Magnetic Model - File Processing Utility: C-Program\n            --- Model Release Year: %d ---\n           --- Software Release Date: %s ---\nUSAGE:\n", (int)MagneticModels[epochs-1]->epoch, VersionDate);
        printf("coordinate file: emm_sph_file f input_file output_file\n");
        printf("or for help:     emm_sph_file h \n");
        printf("\n");
        printf("The input file may have any number of entries but they must follow\n");
        printf("the following format\n");
        printf("Date and location Formats: \n");
        printf("   Date: xxxx.xxx for decimal  (%.1f)\n", MagneticModels[0]->epoch+3.7);
        printf("   Altitude: M - Above mean sea level: E above WGS84 Ellipsoid \n");
        printf("   Altitude: Kxxxxxx.xxx for kilometers  (K1000.13)\n");
        printf("             Mxxxxxx.xxx for meters  (m1389.24)\n");
        printf("             Fxxxxxx.xxx for feet  (F192133.73)\n");
        printf("   Lat/Lon: xxx.xxx in decimal  (-76.53)\n");
        printf("            (Lat and Lon must be specified in the same format.)\n");
        printf("   Date and altitude must fit model.\n");
        printf("   Lat: -90 to 90 (Use - to denote Southern latitude.)\n");
        printf("   Lon: -180 to 180 (Use - to denote Western longitude.)\n");
        printf("   Date: 2010.0 to 2015.0\n");
        printf("   An example of an entry in input file\n");
        printf("   2013.7 E F30000 -70.3 -30.8 \n");
        printf("\n Press enter to exit.");
        printf("\n >");
        fgets(ans, 20, stdin);
            for(Epoch = 0; Epoch < epochs; Epoch++) MAG_FreeMagneticModelMemory(MagneticModels[Epoch]);
    MAG_FreeMagneticModelMemory(MagneticModels[epochs]);
    MAG_FreeMagneticModelMemory(TimedMagneticModel);
        exit(2);
    } /* help */

    if((argv == 4) && (*(args[1]) == 'f'))
    {
        printf("\n\n 'f' switch: converting file with multiple locations.\n");
        printf("     The first five output columns repeat the input coordinates.\n");
        printf("     Then follows D, I, H, X, Y, Z, and F.\n");
        printf("     Finally the SV: Ddot, Idot, Hdot, Xdot, Ydot, Zdot,  and Fdot\n");
        printf("     The units are the same as when the program is\n");
        printf("     run in command line or interactive mode.\n\n");
        coords_from_file = 1;
        strncpy(coord_fname, args[2], MAXREAD);
        coordfile = fopen(coord_fname, "rt");
        strncpy(out_fname, args[3], MAXREAD);
        outfile = fopen(out_fname, "w");
        fprintf(outfile, "Date Coord-System Altitude Latitude Longitude D_deg D_min I_deg I_min H_nT X_nT Y_nT Z_nT F_nT dD/dt_min dI/dt_min dH/dt_nT dX/dt_nT dY/dt_nT dZ/dt_nT dF/dt_nT\n");
    } /* file option */

    if((argv == 5) && (*(args[1]) == 'f') && (*(args[4]) == 'g'))
    {
        printf("\n\n 'f' switch: converting file with multiple locations.\n");
        printf("     The first five output columns repeat the input coordinates.\n");
        printf("     Then follows D, I, H, X, Y, Z, and F.\n");
        printf("     Finally the SV: Ddot, Idot, Hdot, Xdot, Ydot, Zdot,  and Fdot\n");
        printf("     The units are the same as when the program is\n");
        printf("     run in command line or interactive mode.\n\n");
        printf("\n  'g' switch: Appends gradients to output file.\n");
        printf("\n  First is the spatial derivative Northward for D, I, H, X, Y, Z, and F.\n");
        printf("\n  Second is the spatial derivative Eastward for D, I, H, X, Y, Z, and F.\n");
        printf("\n  Third is the spatial derivative Downward for D, I, H, X, Y, Z, and F.\n");
        coords_from_file = 1;
        use_gradient = 1;
        strncpy(coord_fname, args[2], MAXREAD);
        coordfile = fopen(coord_fname, "rt");
        strncpy(out_fname, args[3], MAXREAD);
        outfile = fopen(out_fname, "w");
        fprintf(outfile, "Date Coord-System Altitude Latitude Longitude D_deg D_min I_deg I_min H_nT X_nT Y_nT Z_nT F_nT dD/dt_min dI/dt_min dH/dt_nT dX/dt_nT dY/dt_nT dZ/dt_nT dF/dt_nT dX/dx_nt dY/dx_nt dZ/dx_nt dX/dy_nt dY/dy_nt dZ/dy_nt dX/dz_nt dY/dz_nt dZ/dz_nt\n");
    
    }
    
    if(argv >= 2 && argv != 4 && argv != 5)
    {
        printf("\n\nERROR in 'f' switch option: wrong number of arguments\n");
        exit(2);
    }

    if(argv == 1)
    {
        printf("\n\nERROR in switch option: wrong number of arguments\n");
        exit(2);
    }



    while(again == 1)
    {
        if(coords_from_file)
        {
            argv = 6;
            read_flag = fscanf(coordfile, "%s%s%s%s%s%*[^\n]", args[1], args[2], args[3], args[4], args[5]);
            if(read_flag == EOF) goto reached_EOF;
            fprintf(outfile, "%s %s %s %s %s ", args[1], args[2], args[3], args[4], args[5]);
            fflush(outfile);
            iline++;
        } /* coords_from_file */

        /* Switch on how many arguments are supplied. */
        /* Note that there are no 'breaks' after the cases, so these are entry points */
        switch(argv) {
            case 6: strncpy(inbuff, args[5], MAXREAD);
                if((rest = strchr(inbuff, ','))) /* If it contains a comma */
                {
                    decdeg = 2; /* Then not decimal degrees */
                    begin = inbuff;
                    rest[0] = '\0'; /* Chop into sub string */
                    rest++; /* Move to next substring */
                    ilon_deg = atoi(begin);
                    begin = rest;
                    if((rest = strchr(begin, ',')))
                    {
                        rest[0] = '\0';
                        rest++;
                        ilon_min = atoi(begin);
                        ilon_sec = atoi(rest);
                    } else
                    {
                        ilon_min = 0;
                        ilon_sec = 0;
                    }
                } else
                {
                    decdeg = 1; /* Else it's decimal */
                    longitude = atof(args[5]);
                }

            case 5: strncpy(inbuff, args[4], MAXREAD);
                if((rest = strchr(inbuff, ',')))
                {
                    decdeg = 2;
                    begin = inbuff;
                    rest[0] = '\0';
                    rest++;
                    ilat_deg = atoi(begin);
                    begin = rest;
                    if((rest = strchr(begin, ',')))
                    {
                        rest[0] = '\0';
                        rest++;
                        ilat_min = atoi(begin);
                        ilat_sec = atoi(rest);
                    } else
                    {
                        ilat_min = 0;
                        ilat_sec = 0;
                    }
                } else
                {
                    decdeg = 1;
                    latitude = atof(args[4]);
                }

            case 4: strncpy(inbuff, args[3], MAXREAD);
                inbuff[0] = toupper(inbuff[0]);
                if(inbuff[0] == 'K') units = 1;
                else if(inbuff[0] == 'M') units = 2;
                else if(inbuff[0] == 'F') units = 3;
                if(strlen(inbuff) > 1)
                {
                    inbuff[0] = '\0';
                    begin = inbuff + 1;
                    alt = atof(begin);
                }

            case 3: strncpy(inbuff, args[2], MAXREAD);
                inbuff[0] = toupper(inbuff[0]);
                if(inbuff[0] == 'M') igdgc = 1; /* height is above  mean sea level*/
                else if(inbuff[0] == 'E') igdgc = 2; /* height is above  WGS 84 ellepsoid */


            case 2: strncpy(inbuff, args[1], MAXREAD);
                if((rest = strchr(inbuff, '-'))) /* If it contains a dash */
                {
                    range = 2; /* They want a range */
                    rest[0] = '\0'; /* Sep dates */
                    rest++;
                    begin = rest;
                    if((rest = strchr(begin, '-'))) /* If it contains 2 dashs */
                    {
                        rest[0] = '\0'; /* Sep step */
                        rest++;
                        step = atof(rest); /* Get step size */
                    }
                    if((rest = strchr(begin, ','))) /* If it contains a comma */
                    {
                        decyears = 2; /* It's not decimal years */
                        rest[0] = '\0';
                        rest++;
                        ieyear = atoi(begin);
                        begin = rest;
                        if((rest = strchr(begin, ',')))
                        {
                            rest[0] = '\0';
                            rest++;
                            iemonth = atoi(begin);
                            ieday = atoi(rest);
                        } else
                        {
                            iemonth = 0;
                            ieday = 0;
                        }
                        if((rest = strchr(inbuff, ',')))
                        {
                            begin = inbuff;
                            rest[0] = '\0';
                            rest++;
                            isyear = atoi(begin);
                            begin = rest;
                            if((rest = strchr(begin, ',')))
                            {
                                rest[0] = '\0';
                                rest++;
                                ismonth = atoi(begin);
                                isday = atoi(rest);
                            } else
                            {
                                ismonth = 0;
                                isday = 0;
                            }
                        } else
                        {
                            sdate = atof(inbuff);
                        }
                    } else
                    {
                        decyears = 1; /* Else it's decimal years */
                        sdate = atof(inbuff);
                        edate = atof(begin);
                    }
                } else
                {
                    range = 1;
                    if((rest = strchr(inbuff, ','))) /* If it contains a comma */
                    {
                        decyears = 2; /* It's not decimal years */
                        begin = inbuff;
                        rest[0] = '\0';
                        rest++;
                        isyear = atoi(begin);
                        begin = rest;
                        if((rest = strchr(begin, ',')))
                        {
                            rest[0] = '\0';
                            rest++;
                            ismonth = atoi(begin);
                            isday = atoi(rest);
                        } else
                        {
                            ismonth = 0;
                            isday = 0;
                        }
                    } else
                    {
                        decyears = 1; /* Else it's decimal years */
                        sdate = atof(args[1]);
                    }
                }
                if(sdate == 0)
                { /* If date not valid */
                    decyears = -1;
                    range = -1;
                }
                break;

        }

        if(range == 2 && coords_from_file)
        {
            printf("Error in line %1d, date = %s: date ranges not allowed for file option\n\n", iline, args[1]);
            exit(2);
        }

        /*  Obtain the desired model file and read the data  */

        /* if date specified in command line then warn if past end of validity */

        /*  Take in field data  */

        /* Get date */

        if(coords_from_file && !arg_err && (decyears != 1 && decyears != 2))
        {
            printf("\nError: unrecognized date %s in coordinate file line %1d\n\n", args[1], iline);
            arg_err = 1;
        }

        if(coords_from_file && !arg_err && range != 1)
        {
            printf("\nError: unrecognized date %s in coordinate file line %1d\n\n", args[1], iline);
            arg_err = 1;
        }

        if(coords_from_file && !arg_err && (sdate < minyr || sdate > maxyr))
        {
            printf("\nWarning:  date out of range in coordinate file line %1d\n\n", iline);
            printf("\nExpected range = %6.1lf - %6.1lf, entered %6.1lf\n", minyr, maxyr, sdate);
        }


        /* Get altitude min and max for selected model. */
        minalt = -10; /* To be defined */
        maxalt = 1000;

        /* Get Coordinate prefs */

        if(coords_from_file && !arg_err && (igdgc != 1 && igdgc != 2))
        {
            printf("\nError: Unrecognized height reference %s in coordinate file line %1d\n\n", args[1], iline);
            arg_err = 1;
        }

        /* If needed modify height referencing */
        if(igdgc == 2)
        {
            Geoid.UseGeoid = 0; /* height above WGS-84 Ellipsoid */
        } else if(igdgc == 1)
        {
            Geoid.UseGeoid = 1; /* height above MSL */
        }


        /* Do unit conversions if neccessary */
        if(units == 2)
        {
            minalt *= 1000.0;
            maxalt *= 1000.0;
        } else if(units == 3)
        {
            minalt *= 3280.0839895;
            maxalt *= 3280.0839895;
        }

        /* Get altitude */

        if(coords_from_file && !arg_err && (alt < minalt || alt > maxalt))
        {
            printf("\nError: unrecognized altitude %s in coordinate file line %1d\n\n", args[3], iline);
            arg_err = 1;
        }

        /* Convert altitude to km */
        if(units == 2)
        {
            alt *= 0.001;
        } else if(units == 3)
        {
            alt /= 3280.0839895;
        }



        /* Get lat/long prefs */

        if(coords_from_file && !arg_err && decdeg != 1)
        {
            printf("\nError: unrecognized lat %s or lon %s in coordinate file line %1d\n\n", args[4], args[5], iline);
            arg_err = 1;
        }



        /* Get lat/lon */


        /** This will compute everything needed for 1 point in time. **/

        CoordGeodetic.lambda = longitude;
        CoordGeodetic.phi = latitude;
        CoordGeodetic.HeightAboveGeoid = alt;
        UserDate.DecimalYear = sdate;


        Epoch = ((int) UserDate.DecimalYear - MagneticModels[0]->epoch);
        if(Epoch < 0) Epoch = 0;
        if(Epoch > epochs - 1) Epoch = epochs - 1;
        if(LoadedEpoch != Epoch)
        {
            MagneticModels[epochs]->epoch = MagneticModels[Epoch]->epoch;
            MAG_AssignMagneticModelCoeffs(MagneticModels[epochs], MagneticModels[Epoch], MagneticModels[Epoch]->nMax, MagneticModels[Epoch]->nMaxSecVar);
            LoadedEpoch = Epoch;
        }
        MAG_ConvertGeoidToEllipsoidHeight(&CoordGeodetic, &Geoid); /*This converts the height above mean sea level to height above the WGS-84 ellipsoid*/
        MAG_GeodeticToSpherical(Ellip, CoordGeodetic, &CoordSpherical); /*Convert from geodetic to Spherical Equations: 17-18, WMM Technical report*/

        MAG_TimelyModifyMagneticModel(UserDate, MagneticModels[epochs], TimedMagneticModel); /* Time adjust the coefficients, Equation 19, WMM Technical report */
        MAG_Geomag(Ellip, CoordSpherical, CoordGeodetic, TimedMagneticModel, &GeoMagneticElements); /* Computes the geoMagnetic field elements and their time change*/
        MAG_CalculateGridVariation(CoordGeodetic, &GeoMagneticElements);




        /** Above will compute everything for 1 point in time.**/


        /*  Output the final results. */


        if(coords_from_file)
        {
            if(use_gradient)
            {
                MAG_Gradient(Ellip, CoordGeodetic, TimedMagneticModel, &Gradient);
                print_result_file_gradient(outfile,
                    GeoMagneticElements,
                        Gradient);
            }
            else
            {
            print_result_file(outfile,
                    GeoMagneticElements.Decl,
                    GeoMagneticElements.Incl,
                    GeoMagneticElements.H,
                    GeoMagneticElements.X,
                    GeoMagneticElements.Y,
                    GeoMagneticElements.Z,
                    GeoMagneticElements.F,
                    60 * GeoMagneticElements.Decldot,
                    60 * GeoMagneticElements.Incldot,
                    GeoMagneticElements.Hdot,
                    GeoMagneticElements.Xdot,
                    GeoMagneticElements.Ydot,
                    GeoMagneticElements.Zdot,
                    GeoMagneticElements.Fdot);
            }
        }

        if(coords_from_file)
            again = !feof(coordfile) && !arg_err;

        if(again == 1)
        {
            /* Reset defaults to catch on all while loops */
            igdgc = decyears = units = decdeg = -1;
            ismonth = isday = isyear = sdate = edate = range = step = -1;
            latitude = ilat_deg = ilat_min = ilat_sec = 200;
            longitude = ilon_deg = ilon_min = ilon_sec = 200;
            alt = -9999999;
            argv = 1;
        }
    } /* while (again == 1) */

reached_EOF:

    if(coords_from_file) printf("\n Processed %1d lines\n\n", iline);

    if(coords_from_file && !feof(coordfile) && arg_err) printf("Terminated prematurely due to argument error in coordinate file\n\n");


    fclose(coordfile);
    fclose(outfile);

    for(Epoch = 0; Epoch < epochs; Epoch++) MAG_FreeMagneticModelMemory(MagneticModels[Epoch]);
    MAG_FreeMagneticModelMemory(MagneticModels[epochs]);
    MAG_FreeMagneticModelMemory(TimedMagneticModel);


    return 0;
}

void print_result_file(FILE *outf, double d, double i, double h, double x, double y, double z, double f,
        double ddot, double idot, double hdot, double xdot, double ydot, double zdot, double fdot)
{
    int ddeg, ideg;
    double dmin, imin;

    /* Change d and i to deg and min */

    ddeg = (int) d;
    dmin = (d - (double) ddeg)*60;
    if(ddeg != 0) dmin = fabs(dmin);
    ideg = (int) i;
    imin = (i - (double) ideg)*60;
    if(ideg != 0) imin = fabs(imin);

    if(isnan(d))
    {
        if(isnan(x))
            fprintf(outf, " NaN        %4dd %2.0fm  %8.1f      NaN      NaN %8.1f %8.1f", ideg, imin, h, z, f);
        else
            fprintf(outf, " NaN        %4dd %2.0fm  %8.1f %8.1f %8.1f %8.1f %8.1f", ideg, imin, h, x, y, z, f);
    } else
        fprintf(outf, " %4dd %2.0fm  %4dd %2.0fm  %8.1f %8.1f %8.1f %8.1f %8.1f", ddeg, dmin, ideg, imin, h, x, y, z, f);

    if(isnan(ddot))
    {
        if(isnan(xdot))
            fprintf(outf, "      NaN  %7.1f     %8.1f      NaN      NaN %8.1f %8.1f\n", idot, hdot, zdot, fdot);
        else
            fprintf(outf, "      NaN  %7.1f     %8.1f %8.1f %8.1f %8.1f %8.1f\n", idot, hdot, xdot, ydot, zdot, fdot);
    } else
        fprintf(outf, " %7.1f   %7.1f     %8.1f %8.1f %8.1f %8.1f %8.1f\n", ddot, idot, hdot, xdot, ydot, zdot, fdot);
    return;
} /* print_result_file */

void print_result_file_gradient(FILE *outf, MAGtype_GeoMagneticElements Output, MAGtype_Gradient OutputGradient)
{
    int ddeg, ideg;
    double dmin, imin;

    /* Change d and i to deg and min */

    ddeg = (int) Output.Decl;
    dmin = (Output.Decl - (double) ddeg)*60;
    if(ddeg != 0) dmin = fabs(dmin);
    ideg = (int) Output.Incl;
    imin = (Output.Incl - (double) ideg)*60;
    if(ideg != 0) imin = fabs(imin);

    if(isnan(Output.Decl))
    {
        if(isnan(Output.X))
            fprintf(outf, " NaN        %4dd %2.0fm  %8.1f      NaN      NaN %8.1f %8.1f", ideg, imin, Output.H, Output.Z, Output.F);
        else
            fprintf(outf, " NaN        %4dd %2.0fm  %8.1f %8.1f %8.1f %8.1f %8.1f", ideg, imin, Output.H, Output.X, Output.Y, Output.Z, Output.F);
    } else
        fprintf(outf, " %4dd %2.0fm  %4dd %2.0fm  %8.1f %8.1f %8.1f %8.1f %8.1f", ddeg, dmin, ideg, imin, Output.H, Output.X, Output.Y, Output.Z, Output.F);

    if(isnan(Output.Decldot))
    {
        if(isnan(Output.Xdot))
            fprintf(outf, "      NaN  %7.1f     %8.1f      NaN      NaN %8.1f %8.1f", Output.Incldot * 60, Output.Hdot, Output.Zdot, Output.Fdot);
        else
            fprintf(outf, "      NaN  %7.1f     %8.1f %8.1f %8.1f %8.1f %8.1f", Output.Incldot * 60, Output.Hdot, Output.Xdot, Output.Ydot, Output.Zdot, Output.Fdot);
    } else
        fprintf(outf, " %7.1f   %7.1f     %8.1f %8.1f %8.1f %8.1f %8.1f", Output.Decldot * 60, Output.Incldot * 60, Output.Hdot, Output.Xdot, Output.Ydot, Output.Zdot, Output.Fdot);
    
    fprintf(outf, " %8.1lf %8.1lf %8.1lf %8.1lf %8.1lf %8.1lf %8.1lf %8.1lf %8.1lf\n", OutputGradient.GradPhi.X , OutputGradient.GradPhi.Y, OutputGradient.GradPhi.Z, OutputGradient.GradLambda.X, OutputGradient.GradLambda.Y, OutputGradient.GradLambda.Z, OutputGradient.GradZ.X, OutputGradient.GradZ.Y, OutputGradient.GradZ.Z);
    return;
} /* print_result_file */

/****************************************************************************/
/*                                                                          */
/*                       Subroutine degrees_to_decimal                      */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Converts degrees,minutes, seconds to decimal degrees.                */
/*                                                                          */
/*     Input:                                                               */
/*            degrees - Integer degrees                                     */
/*            minutes - Integer minutes                                     */
/*            seconds - Integer seconds                                     */
/*                                                                          */
/*     Output:                                                              */
/*            decimal - degrees in decimal degrees                          */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 12, 1988                                                */
/*                                                                          */

/****************************************************************************/

double degrees_to_decimal(int degrees, int minutes, int seconds)
{
    double deg;
    double min;
    double sec;
    double decimal;

    deg = degrees;
    min = minutes / 60.0;
    sec = seconds / 3600.0;

    decimal = fabs(sec) + fabs(min) + fabs(deg);

    if(deg < 0)
    {
        decimal = -decimal;
    } else if(deg == 0)
    {
        if(min < 0)
        {
            decimal = -decimal;
        } else if(min == 0)
        {
            if(sec < 0)
            {
                decimal = -decimal;
            }
        }
    }

    return (decimal);
}

/****************************************************************************/
/*                                                                          */
/*                           Subroutine julday                              */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/*     Computes the decimal day of year from month, day, year.              */
/*     Leap years accounted for 1900 and 2000 are not leap years.           */
/*                                                                          */
/*     Input:                                                               */
/*           year - Integer year of interest                                */
/*           month - Integer month of interest                              */
/*           day - Integer day of interest                                  */
/*                                                                          */
/*     Output:                                                              */
/*           date - Julian date to thousandth of year                       */
/*                                                                          */
/*     FORTRAN                                                              */
/*           S. McLean                                                      */
/*           NGDC, NOAA egc1, 325 Broadway, Boulder CO.  80301              */
/*                                                                          */
/*     C                                                                    */
/*           C. H. Shaffer                                                  */
/*           Lockheed Missiles and Space Company, Sunnyvale CA              */
/*           August 12, 1988                                                */
/*                                                                          */
/*     Julday Bug Fix                                                       */
/*           Thanks to Rob Raper                                            */

/****************************************************************************/


double julday(i_month, i_day, i_year)
int i_month;
int i_day;
int i_year;
{
    int aggregate_first_day_of_month[13];
    int leap_year = 0;
    int truncated_dividend;
    double year;
    double day;
    double decimal_date;
    double remainder = 0.0;
    double divisor = 4.0;
    double dividend;
    double left_over;

    aggregate_first_day_of_month[1] = 1;
    aggregate_first_day_of_month[2] = 32;
    aggregate_first_day_of_month[3] = 60;
    aggregate_first_day_of_month[4] = 91;
    aggregate_first_day_of_month[5] = 121;
    aggregate_first_day_of_month[6] = 152;
    aggregate_first_day_of_month[7] = 182;
    aggregate_first_day_of_month[8] = 213;
    aggregate_first_day_of_month[9] = 244;
    aggregate_first_day_of_month[10] = 274;
    aggregate_first_day_of_month[11] = 305;
    aggregate_first_day_of_month[12] = 335;

    /* Test for leap year.  If true add one to day. */

    year = i_year; /*    Century Years not   */
    if((i_year != 1900) && (i_year != 2100)) /*  divisible by 400 are  */
    { /*      NOT leap years    */
        dividend = year / divisor;
        truncated_dividend = dividend;
        left_over = dividend - truncated_dividend;
        remainder = left_over*divisor;
        if((remainder > 0.0) && (i_month > 2))
        {
            leap_year = 1;
        } else
        {
            leap_year = 0;
        }
    }
    day = aggregate_first_day_of_month[i_month] + i_day - 1 + leap_year;
    if(leap_year)
    {
        decimal_date = year + (day / 366.0); /*In version 3.0 this was incorrect*/
    } else
    {
        decimal_date = year + (day / 365.0); /*In version 3.0 this was incorrect*/
    }
    return (decimal_date);
}

