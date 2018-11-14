# README #

This code is associated with the Science Advances research article, "Bioinspired polarization vision enables underwater geolocalization" [DOI: 10.1126/sciadv.aao6841](http://dx.doi.org/10.1126/sciadv.aao6841) If you use this code, or any portion thereof, for research purposes please cite:

### List of files ###

+ Polarization video processing:
	+ polprocess.py - calibrate and compute stokes vectors for polarization videos
	+ make_cal.py - compute calibration files for polarization camera
	+ crop_cal.py - resize calibration files for a particular ROI
+ Modeling underwater polarization states:
	+ polarization.py - general Stokes vector & Mueller matrix routines
	+ simulate.py - Simulate a single-scattering event underwater
+ Underwater navigation:
	+ estimate_position.py - do position estimation
	+ sensitivity.py - sensitivity statistics
	+ sunpos/* - Cython implementation of Reda & Andreas's 2004 sun position algorithm
	+ geomag/* - Cython wrapper for NOAA's EMM 2015-2020
+ Statistics and figure preparation routines
	+ error_stats.py
	+ kent_distribution.py - From https://github.com/edfraenkel/kent_distribution
	+ model_aop_figure.py
	+ plot_utils.py
	+ stats_utils.py

### Dependencies ###

* Python 3+, Cython
* Scipy, Numpy, Matplotlib, Numexpr, Ipython, h5py
* Basemap, Geographiclib, Pint
