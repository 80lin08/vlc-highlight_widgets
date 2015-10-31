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

General patch works with VLC 2.1+ and tested with 2.2.1
Patch is tested for 2.2.1, but, according to VideoLAN git, it works with latest version as well.
I suppose, it should be suitable for use with VLC 3.x too
