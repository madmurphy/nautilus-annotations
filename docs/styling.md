Styling
=======

**Nautilus Annotations** declares three unique CSS classes:

* `dialog.nautilus-annotations-dialog` (the annotations window)
* `textview.nautilus-annotations-view` (the annotations text area)
* `button.nautilus-annotations-discard` (the button for discarding the current
   changes)

These are meant to be used in the first place by the extension's main CSS
(`/usr/share/nautilus-annotations/style.css`), but can be completed by themes
or overridden by user-given style sheets.

Widgets without a class can be styled via CSS inheritance. The
`.nautilus-annotations-dialog` window is populated with the following DOM tree:

	dialog.nautilus-annotations-dialog
	│
	├── headerbar.titlebar
	│   │
	│   ├── box
	│   │   │
	│   │   │── label.title
	│   │   │
	│   │   ╰── label.subtitle
	│   │
	│   │── button.text-button.nautilus-annotations-discard
	│   │   │
	│   │   ╰── label
	│   │
	│   ╰── button.titlebutton.close
	│       │
	│       ╰── image
	│
	╰── box.dialog-vbox
	    │
	    ╰── scrolledwindow
	        │
	        ╰── textview.view.sourceview.nautilus-annotations-view

It is possible to experiment interactively with GTK and CSS using
**GtkInspector**. For experimenting on this extension, launch

``` sh
nautilus -q
GTK_DEBUG=interactive nautilus
```

For example, by pasting the following style sheet into **GtkInspector**'s “CSS”
tab, the annotations will be styled as yellow sticky notes:

``` css
dialog.nautilus-annotations-dialog,
dialog.nautilus-annotations-dialog .titlebar {
	border: none;
	background-image: none;
	background-color: #fff394;
	color: #ad5f00;
}

dialog.nautilus-annotations-dialog selection {
	background-image: none;
	background-color: #ad5f00;
	color: #fff394;
}

dialog.nautilus-annotations-dialog text,
dialog.nautilus-annotations-dialog button {
	background-image: none;
	background-color: transparent;
	color: #ad5f00;
}

dialog.nautilus-annotations-dialog button.close {
	color: #cc0000;
	border: none;
	box-shadow: none;
}

dialog.nautilus-annotations-dialog button.nautilus-annotations-discard {
	padding: 0 6px;
	background-color: #ad5f00;
	color: #fff394;
	border: none;
	box-shadow: 0 0 2px 2px #ad5f00;
}

textview.nautilus-annotations-view {
	font-family: monospace;
	padding: 0;
}
```

Under `css-factory` a collection of CSS themes is available. If you have new
stylesheets and ideas, please do not hesitate to propose them via
[merge request][1] or [message][2].

For more information, please refer to the [GTK CSS documentation][3].


  [1]: https://gitlab.gnome.org/madmurphy/nautilus-annotations
  [2]: https://gitlab.gnome.org/madmurphy/nautilus-annotations/issues
  [3]: https://docs.gtk.org/gtk3/css-overview.html

