simtoi
======

The SImulation and Modeling Tool for Optical Interferometry

## Building instructions

```
cd build
cmake ..
make
```

If you encounter errors see the wiki for assistance.

## External Libraries

At a minimum, these libraries must be installed on your system:
* CFITSIO
* Some OpenCL library

SIMTOI includes the following libraries as part of its distribution:
* [OpenCL Interferometry Library (liboi)](https://github.com/bkloppenborg/liboi_
* [json-cpp](http://sourceforge.net/projects/jsoncpp/)
* [levmar](http://www.ics.forth.gr/~lourakis/levmar/)

Due to licensing the following libraries must be downloaded and installed by the user before they may be used in SIMTOI
* [MultiNest](http://ccpforge.cse.rl.ac.uk/gf/project/multinest/)

## Licensing and Acknowledgements

SIMTOI is free software, distributed under the [GNU Lesser General Public License (Version 3)](<http://www.gnu.org/licenses/lgpl.html). 

If you use this software as part of a scientific publication, please cite the following works:

Kloppenborg, B.; Baron, F. (2012) "SIMTOI: SImulation and Modeling Tool for Optical Interferometry" (Version X).  Available from  <https://github.com/bkloppenborg/simtoi>.

Kloppenborg, B.; Baron, F. (2012), "LibOI: The OpenCL Interferometry Library"
(Version X). Available from  <https://github.com/bkloppenborg/liboi>.

If you use the _levmar_ minmizer, see their instruction for including [a suitable reference](http://www.ics.forth.gr/~lourakis/levmar/bibentry.html).

If you use the _MultiNest_ minimizer see reference instructions on the [_MultiNest_ website](http://ccpforge.cse.rl.ac.uk/gf/project/multinest/)
