Term Reference Tree
======================

Term Reference Tree provides an expandable tree widget for the Taxonomy Term Reference field in Backdrop. This widget is intended to serve as a replacement for Backdrop's core Taxonomy Term Reference widget, which is a flat list of radio buttons or checkboxes and not necessarily fit for medium to large taxonomy trees.

This widget has the following features:

* Expand/minimize buttons
* Fully themeable
* Filter and sort available options with a View
* The ability to start with the tree either minimized or maximized
* If you limit the number of selectable options, client-side Javascript limits the number of terms that can be selected by disabling the other remaining options when the limit has been reached (this is enforced on the server side too).
* For large trees, this widget now optionally keeps a list of selected items below the tree.
* You can use tokens to alter the widget label (good for adding icons, turning the options into links, etc).

This module comes with a display formatter with the following features:

* Display taxonomy terms as a nested list by hierarchy.
* Displayed terms can be altered with tokens or themed using a custom theme function.

Installation
------------

Install this module using [the official Backdrop CMS instructions](  https://backdropcms.org/guide/modules).

1. Go to the Manage Fields tab of any fieldable entity, such as a content type, taxonomy term, or user.
2. Add a new field with a type of Term Reference.
3. In the widget select box, select Term reference tree as the Widget Type.

Note that you can also change any Term Reference field to use the Term Reference Tree widget, by editing that field and selecting Term Reference Tree on that field's Widget Type tab.

If you want to filter by a view, make sure the view includes the taxonomy term ID, and that the items per page is set to unlimited, as the widget will only display the items from the first page of results.

This module is also perfectly good for flat lists, particularly if you want to filter them by a view or limit the number of selectable options with JavaScript. It's a good idea to turn on the "leaves only" option in this case, as it will make it look nicer.

Documentation
-------------

Additional documentation is located in [the Wiki](https://github.com/backdrop-contrib/term_reference_tree/wiki/Documentation).

Differences from Drupal 7
-------------------------

The Backdrop version of this module has some additional classes applied to the `<li>` elements of the rendered tree to give more flexibility in theming.

Issues
------

Bugs and feature requests should be reported in [the Issue Queue](https://github.com/backdrop-contrib/term_reference_tree/issues).

Current Maintainers
-------------------

- [Robert J. Lang](https://github.com/bugfolder).

Credits
-------

- Ported to Backdrop CMS by [Robert J. Lang](https://github.com/bugfolder).
- Originally written for Drupal by [BartK](https://www.drupal.org/u/bartk).

License
-------

This project is GPL v2 software.
See the LICENSE.txt file in this directory for complete text.

