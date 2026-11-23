# windwm wallpapers: photographed, not generated

These four wallpaper presets are **photos by external photographers** on Wikimedia
Commons and each is published under **Creative Commons CC0 1.0** (a public-domain
dedication). They are not copyrighted images with an unknown/absent license.
CC0 permits redistribution and modification without mandatory attribution; we
still credit the creators and retain source pages below.

| Preset/file | Photographer | Original Wikimedia Commons page | License |
| --- | --- | --- | --- |
| `sunflowers.jpg` | Aleksandr Eremin | [Sunflowers field (Unsplash).jpg](https://commons.wikimedia.org/wiki/File:Sunflowers_field_(Unsplash).jpg) | CC0 1.0 |
| `ridge.jpg` | Kamran Sabir | [Mountain During Sunset-129099.JPG](https://commons.wikimedia.org/wiki/File:Mountain_During_Sunset-129099.JPG) | CC0 1.0 (reviewed by Commons in 2017) |
| `pine.jpg` | Jude Dias | [Thick pine forest (Unsplash).jpg](https://commons.wikimedia.org/wiki/File:Thick_pine_forest_(Unsplash).jpg) | CC0 1.0 |
| `dusk.jpg` | Wilfredor | [Mountain silhouette at sunset in Rio de Janeiro, Brazil.jpg](https://commons.wikimedia.org/wiki/File:Mountain_silhouette_at_sunset_in_Rio_de_Janeiro,_Brazil.jpg) | CC0 1.0 |

All licenses were checked against the individual Wikimedia Commons file pages
on 23 November 2026. The Unsplash originals were released under CC0 before
Unsplash switched to its current custom license in 2017. A later Unsplash
license does not automatically grant the same rights to new uploads.

The source release intentionally **does not bundle the JPEG photographs**.
Run `make wallpapers` (requires `curl`, internet access) to download 1920px
copies directly from Wikimedia Commons. Then run `sudo make install` to copy
wallpapers into `/usr/local/share/windwm/assets/` alongside the program.

You may also choose your own file in `~/.config/wind/config.lua`, e.g.
`wallpaper = "~/Pictures/my-photo.jpg"`.

The windwm program source is MIT-licensed. The four external photographs are
separately CC0; they are **not** covered by the windwm MIT license.
