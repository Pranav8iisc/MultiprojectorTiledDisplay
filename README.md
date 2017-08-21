# MultiprojectorTiledDisplay #
Predominantly, this code is an implementation of the multiprojector geometric alignment and edge blending approaches proposed [here](http://ieeexplore.ieee.org/document/1167859/). Futher, we have [proposed](http://ieeexplore.ieee.org/document/7019727/) and implemented an approach for enhancing the overall projection resolution by utilizing the perspective projection invariant, [cross-ratio](https://en.wikipedia.org/wiki/Cross-ratio). Code has been tested for only one planar projection region configuration till now, a 3X3 multiprojector setup. Work on GUI development and addressing arbitrary tiled configurations is under progress.

### Current limirations
* Hard-coded configuration(see, sl\_multiproj.cpp)
* Tested on older OpenCV only(v2.4)
* Assumes gPhoto2 supported camera


### Dependecies
* OpenCV
* gPhoto2
* Qt(soon...)

### TODOs
* GUI
* User manual(for hackers, sl\_multiproj.cpp contains the core implmentation)
* File(or, GUI)-based configurability 
* Extension of the code for arbitrary planar multiprojector arrangements

### Who do I talk to? ###

* Please write to pranav8iisc@gmail.com for any queries related with development. 
