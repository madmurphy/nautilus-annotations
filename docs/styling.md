Styling
=======

**Nautilus Annotations** provides three CSS classes:

* `dialog.nautilus-dialog-annotations` (the annotations window)
* `textview.nautilus-textarea-annotations` (the annotations text area)
* `button.nautilus-discard-annotations` (the button for discarding current
   changes)

The classes above are meant to be used in the first place by the extension's
main CSS (`/usr/share/nautilus-annotations/style.css`), but can be completed
by themes or overridden by user-given style sheets.

Widgets without a class can be styled via CSS inheritance. The
`.nautilus-dialog-annotations` window is populated with the following DOM tree:

	dialog.nautilus-dialog-annotations
	│
	├── headerbar.titlebar
	│   │
	│   ├── box
	│   │   │
	│   │   │── label.title
	│   │   │
	│   │   ╰── label.subtitle
	│   │
	│   │── button.text-button.nautilus-discard-annotations
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
	        ╰── textview.view.sourceview.nautilus-textarea-annotations

For more information, please refer to the GTK CSS documentation.

