Styling
=======

**Nautilus Annotations** provides four CSS classes:

* `dialog.nautilus-annotations` (the annotation window)
* `textview.nautilus-textarea-annotations` (the annotation text area)
* `dialog.nautilus-okcancel-annotations` (the window asking questions)
* `label.nautilus-question-annotations` (the label containing questions)

The classes above are meant to be used in the first place by the extension's
main CSS (`/usr/share/nautilus-annotations/style.css`), but can be completed
by themes or overridden by user-given style sheets.

Widgets without a class can be styled via CSS inheritance. The
`dialog.nautilus-annotations` window is populated with the following DOM tree:

	dialog.nautilus-annotations
	│
	├── headerbar.titlebar
	│   │
	│   ├── box
	│   │   │
	│   │   ╰── label.title
	│   │
	│   ╰── box
	│       │
	│       ╰── button.titlebutton.close
	│
	╰── box.dialog-vbox
	    │
	    │── scrolledwindow
	    │   │
	    │   ╰── textview.view.sourceview.nautilus-textarea-annotations
	    │
	    ╰── box.dialog-action-box
	        │
	        ╰── buttonbox.dialog-action-area
	            │
	            ╰── button.text-button
	                │
	                ╰── label

The `dialog.nautilus-okcancel-annotations` window is populated with the
following DOM tree:

	dialog.nautilus-okcancel-annotations
	│
	├── headerbar.titlebar
	│   │
	│   ├── box
	│   │   │
	│   │   ╰── label.title
	│   │
	│   ╰── box
	│       │
	│       ╰── button.titlebutton.close
	│
	╰── box.dialog-vbox
	    │
	    │── label.nautilus-question-annotations
	    │
	    ╰── box.dialog-action-box
	        │
	        ╰── buttonbox.dialog-action-area
	            │
	            ├── button.text-button.default
	            │   │
	            │   ╰── label
	            │
	            ╰── button.text-button
	                │
	                ╰── label

For more information, please refer to the GTK CSS documentation.

