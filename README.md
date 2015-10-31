# vlc-highlight_widgets
patch for VLC (by VideoLAN) that adds highlights to wideo widgets

By default, VLC has white video widgets (play/pause icons)
But they are unusable for light video scenes.

This patch makes it black with white highligts (like DMZ-black)



You can apply this patch by executing in VLC folder:
patch -p0 -i vlc_video_widgets_highlights.patch

# Suitability
Patch for older versions:
vlc_video_widgets_highlights-2.0.0.patch 
is suitable for VLC 2.0.x

General patch works with VLC 2.1+ and tested with 2.2.1.

Patch is tested for 2.2.1, but, according to VideoLAN git, it works with latest version as well.

I suppose, it should be suitable for use with VLC 3.x too.

# Screenshots

Widgets before:

![old_0](https://raw.githubusercontent.com/80lin08/vlc-highlight_widgets/master/screenshots/old_0.png)

![old_1](https://raw.githubusercontent.com/80lin08/vlc-highlight_widgets/master/screenshots/old_1.png)

Widgets after:

![new_0](https://raw.githubusercontent.com/80lin08/vlc-highlight_widgets/master/screenshots/new_0.png)

![new_1](https://raw.githubusercontent.com/80lin08/vlc-highlight_widgets/master/screenshots/new_1.png)
