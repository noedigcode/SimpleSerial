Appicon Readme
==============

Easily add an application icon by simply including the .pri file in your Qt
project (.pro) file.

E.g. if this folder is called "appicon", place it in your project root.

Then add this to your .pro file: include(appicon/appicon.pri)

Replace the appicon.ico and appicon.png files with your app icon.

A Windows exe icon will automatically be created for your app when building.

The .png file can be used in your app using the Qt form designer for example,
i.e. as your MainWindow icon.
