Themes
======

This directory provides a selection of themes for **Nautilus Annotations**.

Each theme is contained in a CSS file named `[THEME-NAME]-theme.css`. To set a
theme for the current user launch

``` sh
mkdir -p ~/.local/share/nautilus-annotations/
cp /usr/share/doc/nautilus-annotations/css-factory/[THEME-NAME]-theme.css \
	~/.local/share/nautilus-annotations/style.css
nautilus -q
```

To set a system-wide theme two options are available:

1. Pass a `--with-theme=[THEME-NAME]` option to the `configure` script during
   the build process
2. Replace manually, after the package has been installed, the content of
   `/usr/share/nautilus-annotations/style.css` with that of the selected theme

Whenever the `~/.local/share/nautilus-annotations/style.css` file is present
the `/usr/share/nautilus-annotations/style.css` file is ignored.

Please bear in mind that extension-specific themes have always higher priority
over GTK themes; if you want to style this extension via the latter, or simply
want to experiment with CSS globally, you should not install any theme.

If you have new stylesheets and ideas, please do not hesitate to propose them
via [merge request][1] or [message][2].


  [1]: https://gitlab.gnome.org/madmurphy/nautilus-annotations
  [2]: https://gitlab.gnome.org/madmurphy/nautilus-annotations/issues

