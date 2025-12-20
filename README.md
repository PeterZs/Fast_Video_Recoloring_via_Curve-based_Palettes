## Fast Video Recoloring via Curve-based Palettes

![](https://github.com/Zhengjun-Du/GeometricPaletteBasedVideoRecoloring/blob/main/teaser.png) 需要替换成 overview.pdf 的文件路径

This is the source code of the paper: **Fast Video Recoloring via Curve-based Palettes**, authors: Zheng-Jun Du, Jia-Wei Zhou, Kang Li, Jian-Yu Hao, Zi-Kang Huang, Kun Xu*. Please fell free to contact us if you have any questions, email: duzj19@mails.tsinghua.edu.cn

### Requirements

Windows 10  
Microsoft Visual Studio 2022  
OpenCV 4.1  
Nlopt 2.4.2
Qt 5.12.12

### Directories

1. data: a test video
2. Qt-Color-Widgets：color Picker

### Usage

1. Click "Open Video" to load the video.

2. Click "Extract Palette" to generate the original color palette. You can edit the color palette by selecting key frames using the progress bar in "Video progress".

3. After editing, click “Recolor” to recolor the video.

4. Other features:

   a) click "Reset" to restore all palettes. You can also right-click on the corresponding palette to reset its colors individually.

   b) click "Import Palette", "Export Palette" and "Export Video" to import the color palette, export the color palette, and export the video, respectively. Imported and exported data are stored in "./files.txt".

![](https://github.com/Zhengjun-Du/GeometricPaletteBasedVideoRecoloring/blob/main/recolor-ui.png) 需要替换成 GUI.png 的文件路径

### References

[1] H. Chang, O. Fried, Y. Liu, S. DiVerdi, and A. Finkelstein, “Palette-based photo recoloring.” ACM Trans. Graph., vol. 34, no. 4, pp. 139–1, 2015.

[2] Q. Zhang, C. Xiao, H. Sun, and F. Tang, “Palette-based image recoloring using color decomposition optimization,” IEEE Transactions on Image Processing, vol. 26, no. 4, pp. 1952–1964, 2017.

[3] J. Tan, J.-M. Lien, and Y. Gingold, “Decomposing images into layers via RGB-space geometry,” ACM Transactions on Graphics (TOG), vol. 36, no. 1, pp. 1–14, 2016.