Styling
=======

**Nautilus Annotations** provides three CSS classes:

* `dialog.nautilus-annotations-dialog` (the annotations window)
* `textview.nautilus-annotations-view` (the annotations text area)
* `button.nautilus-annotations-discard` (the button for discarding the current
   changes)

The classes above are meant to be used in the first place by the extension's
main CSS (`/usr/share/nautilus-annotations/style.css`), but can be completed
by themes or overridden by user-given style sheets.

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

For more information, please refer to the [GTK CSS documentation][1].


  [1]: https://developer.gnome.org/gtk3/stable/chap-css-overview.html

