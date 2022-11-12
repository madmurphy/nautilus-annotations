Styling
=======

**Nautilus Annotations** declares four unique CSS classes:

* `window.nautilus-annotations-dialog` (the annotations window)
* `scrolledwindow.nautilus-annotations-scrollable` (the scrollable area)
* `textview.nautilus-annotations-view` (the annotations text area)
* `button.nautilus-annotations-discard` (the button for discarding the current
   changes)

These are meant to be used in the first place by the extension's main CSS
(`/usr/share/nautilus-annotations/style.css`), but can be completed by themes
or overridden by user-given style sheets.

Widgets without a class can be styled via CSS inheritance. The
`.nautilus-annotations-dialog` window is populated by the following DOM tree:

	window.nautilus-annotations-dialog
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
	    ╰── scrolledwindow.nautilus-annotations-scrollable
	        │
	        ╰── textview.view.sourceview.nautilus-annotations-view

It is possible to experiment interactively with GTK and CSS using
**GtkInspector**. For experimenting on this extension, launch

``` sh
nautilus -q
GTK_DEBUG=interactive nautilus
```

For example, by pasting the following style sheet into **GtkInspector**'s “CSS”
tab, the annotations will be styled as yellow sticky notes (please use an
unthemed installation for experimenting with CSS, via
`./configure --without-theme`):

``` css
window.nautilus-annotations-dialog {
	border: none;
	background-image: none;
	background-color: #fff394;
}

window.nautilus-annotations-dialog .titlebar {
	border: none;
	background-image: none;
	background-color: transparent;
	color: #ad5f00;
	box-shadow: none;
}

window.nautilus-annotations-dialog selection {
	background-image: none;
	background-color: #ad5f00;
	color: #fff394;
}

window.nautilus-annotations-dialog text,
window.nautilus-annotations-dialog button {
	background-image: none;
	background-color: transparent;
	color: #ad5f00;
}

window.nautilus-annotations-dialog button.close {
	color: #cc0000;
	border: none;
	box-shadow: none;
}

window.nautilus-annotations-dialog button.nautilus-annotations-discard {
	padding: 0 6px;
	background-color: #ad5f00;
	color: #fff394;
	font-weight: normal;
	border: none;
	box-shadow: 0 0 2px 2px #ad5f00;
}

textview.nautilus-annotations-view {
	font-family: monospace;
	padding: 0;
	background-image: none;
	background-color: transparent;
	color: #ad5f00;
}
```

Under `css-factory` a collection of CSS themes is available. If you have new
stylesheets and ideas, please do not hesitate to propose them via
[merge request][1] or [message][2].

For more information, please refer to the [GTK CSS documentation][3].


  [1]: https://gitlab.gnome.org/madmurphy/nautilus-annotations
  [2]: https://gitlab.gnome.org/madmurphy/nautilus-annotations/issues
  [3]: https://docs.gtk.org/gtk3/css-overview.html

