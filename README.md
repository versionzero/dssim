#RGBA Structural Similarity

This tool computes (dis)similarity between two PNG images using (my approximation of) algorithms approximating human vision.

Comparison is done in L\*a\*b\* color space† using SSIM algorithm.
 
It's a rewrite of [Rabah Mehdi's C++ implementation](http://mehdi.rabah.free.fr/SSIM/):

* No C++ (C99)
* No OpenCV dependency (only `libpng`)
* Supports alpha channel
* Supports gamma correction

Values output are not comparable with other tools. This tool is meant to be fast, and I'm not giving any guarantees about correctness (or speed :)

†) conversion assumes D65 white point and uses gamma from PNG file, defaulting to 2.2.

##Usage

    dssim file.png file-modified.png

Will output something like `0.2341`. 0 means exactly the same image, >0 (unbounded) is amount of difference.

    dssim file.png file-modified.png totally-rad-visualisation.png
  
Will also output map of differences. Transparent areas will have funky pattern added.

