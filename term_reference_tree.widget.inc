<?php

/**
 * Implements hook_field_widget_info().
 */
function term_reference_tree_field_widget_info() {
  return array(
    'term_reference_tree' => array (
      'label' => 'Term reference tree',
      'field types' => array('taxonomy_term_reference'),
      'behaviors' => array(
        'multiple values' => FIELD_BEHAVIOR_CUSTOM, 
        'default value' => FIELD_BEHAVIOR_DEFAULT,
      ),
      'settings' => array(
        'start_minimized' => 0,
        'leaves_only' => 0,
        'filter_view' => '',
        'select_parents' => 0,
        'track_list' => 0,
        'token_display' => '',
      ),
    ),
  );
}

/**
 * Themes the term tree display (as opposed to the select widget).
 */
function theme_term_tree_list($variables) {
  $element =& $variables['element'];
  $data =& $element['#data'];

  $tree = array();

  # For each selected term:
  foreach($data as $item) {
    # Loop if the term ID is not zero:
    $values = array();
    $tid = $item['tid'];
    $original_tid = $tid;
    while($tid != 0) {
      # Unshift the term onto an array
      array_unshift($values, $tid);
      
      # Repeat with parent term
      $tid = _term_reference_tree_get_parent($tid);
    }
    
    $current =& $tree;
    # For each term in the above array:
    foreach($values as $tid) {
      # current[children][term_id] = new array
      if (!isset($current['children'][$tid])) {
        $current['children'][$tid] = array('selected' => FALSE);
      }
      
      # If this is the last value in the array, tree[children][term_id][selected] = true
      if ($tid == $original_tid) {
        $current['children'][$tid]['selected'] = TRUE;
      }

      $current['children'][$tid]['tid'] = $tid;
      $current =& $current['children'][$tid];
    }
  }
  
  return _term_reference_tree_output_list_level($element, $tree);
}


/**
 * Implements hook_field_widget_settings_form().
 */
function term_reference_tree_field_widget_settings_form($field, $instance) {
  $widget = $instance['widget'];
  $settings = $widget['settings'];
  $form = array();
  
  if ($widget['type'] == 'term_reference_tree') {
    $form['start_minimized'] = array(
      '#type' => 'checkbox',
      '#title' => t('Start minimized'),
      '#description' => t('Make the tree appear minimized on the form by default'),
      '#default_value' => $settings['start_minimized'],
      '#return_value' => 1,
    );
    
    $form['leaves_only'] = array(
      '#type' => 'checkbox',
      '#title' => t('Leaves only'),
      '#description' => t("Don't allow the user to select items that have children"),
      '#default_value' => $settings['leaves_only'],
      '#return_value' => 1,
    );

    $form['select_parents'] = array(
      '#type' => 'checkbox',
      '#title' => t('Select parents automatically'),
      '#description' => t("When turned on, this option causes the widget to automatically select the ancestors of all selected items. In Leaves Only mode, the parents will be added invisibly to the selected value.  <em>This option is only valid if an unlimited number of values can be selected.</em>"),
      '#default_value' => $settings['select_parents'],
      '#element_validate' => array('_term_reference_tree_select_parents_validate'),
      '#return_value' => 1,
    );

    if (module_exists('views')) {
      $views = views_get_all_views();
      $options = array('' => 'none');
      
      foreach($views as $name => $view) {
        if ($view->base_table == 'taxonomy_term_data') {
          foreach($view->display as $display) {
            $options["$name:{$display->id}"] = "{$view->human_name}: {$display->display_title}";
          }
        }
      }

      $form['filter_view'] = array(
        '#type' => 'select',
        '#title' => 'Filter by view',
        '#description' => t("Filter the available options based on whether they appear in the selected view."),
        '#default_value' => $settings['filter_view'],
        '#options' => $options,
      );
    }
    else {
      $form['filter_view'] = array(
        '#type' => 'hidden',
        '#value' => $settings['filter_view'],
      );
    }

    if (module_exists('token')) {
      $form['token_display'] = array(
        '#type' => 'textarea',
        '#title' => 'Custom Term Label',
        '#description' => t("Use tokens to change the term labels for the checkboxes and/or radio buttons.  Leave this field blank to use the term name."),
        '#default_value' => $settings['token_display'],
      );

      $form['tokens_list'] = array(
        '#theme' => 'token_tree',
        '#token_types' => array('term'),
      );
    }
    else {
      $form['token_display'] = array(
        '#type' => 'hidden',
        '#value' => $settings['token_display'],
      );
    }

    
    $form['track_list'] = array(
      '#type' => 'checkbox',
      '#title' => t('Track list'),
      '#description' => t(
          'Track what the user has chosen in a list below the tree.
           Useful when the tree is large, with many levels.'),
      '#default_value' => $settings['track_list'],
      '#return_value' => 1,
    );
  }

  return $form;
}

/**
 * Helper function to output a single level of the term reference tree
 * display.
 */
function _term_reference_tree_output_list_level(&$element, &$tree) {
  if (isset($tree['children']) && is_array($tree['children']) && count($tree['children'] > 0)) {
    $output = '<ul class="term">';
    $settings = $element['#display']['settings'];
    $tokens_selected = $settings['token_display_selected'];
    $tokens_unselected = ($settings['token_display_unselected'] != '') ? $settings['token_display_unselected'] : $tokens_selected;

    foreach($tree['children'] as &$item) {
      $term = taxonomy_term_load($item['tid']);
      $uri = taxonomy_term_uri($term);
      $class = $item['selected'] ? 'selected' : 'unselected';
      $output .= "<li class='$class'>";
      if ($tokens_selected != '' && module_exists('token')) {
        $replace = $item['selected'] ? $tokens_selected : $tokens_unselected;
        $output .= token_replace($replace, array('term' => $term), array('clear' => TRUE));
      }
      else {
        $output .= l(entity_label('taxonomy_term', $term), $uri['path'], array('html' => true));
      }
      if (isset($item['children'])) {
        $output .= _term_reference_tree_output_list_level($element, $item);
      }
      $output .= "</li>";
    }

    $output .= '</ul>';
    return $output;
  }
}


/**
 * Makes sure that cardinality is unlimited if auto-select parents is enabled.
 */
function _term_reference_tree_select_parents_validate($element, &$form_state) {
  if ($form_state['values']['instance']['widget']['settings']['select_parents'] == 1 && $form_state['values']['field']['cardinality'] != -1) {
    // This is pretty wonky syntax for the field name in form_set_error, but it's
    // correct.
    form_set_error('field][cardinality', t('You must select an Unlimited number of values if Select Parents Automatically is enabled.'));
  }
}

/**
 * Process the checkbox_tree widget.
 * 
 * This function processes the checkbox_tree widget.
 * 
 * @param $element
 *   The element to be drawn.$element['#field_name']
 * @param $form_state
 *   The form state.
 * 
 * @return
 *   The processed element.
 */
function term_reference_tree_process_checkbox_tree($element, $form_state) {
  if (is_array($form_state)) {
    if (!empty($element['#max_choices']) && $element['#max_choices'] != '-1')
      drupal_add_js(array('term_reference_tree' => array('trees' => array($element['#id'] => array('max_choices'=>$element['#max_choices'])))), 'setting');
    $allowed = '';
    
    if ($element['#filter_view'] != '') {
      $allowed = _term_reference_tree_get_allowed_values($element['#filter_view']);
    }

    if (empty($element['#options'])) {
      $element['#options_tree'] = _term_reference_tree_get_term_hierarchy($element['#parent_tid'], $element['#vocabulary'], $allowed, $element['#filter_view'], '');

      $required = $element['#required'];
      if ($element['#max_choices'] == 1 && !$required) {
        array_unshift($element['#options_tree'], (object) array(
          'tid' => '',
          'name' => 'N/A',
          'depth' => 0
          )
        );
      }
      $element['#options'] = _term_reference_tree_get_options($element['#options_tree'], $allowed, $element['#filter_view']);
    }

    $value = !empty($element['#default_value']) ? $element['#default_value'] : array();
    $terms = !empty($element['#options_tree']) ? $element['#options_tree'] : array();
    $max_choices = !empty($element['#max_choices']) ? $element['#max_choices'] : 1;
    if (array_key_exists('#select_parents', $element) && $element['#select_parents']) {
      $element['#attributes']['class'][] = 'select-parents';
    }

    if ($max_choices != 1)
      $element['#tree'] = TRUE;

    $tree = new stdClass;
    $tree->children = $terms;
    $element[] = _term_reference_tree_build_level($element, $tree, $form_state, $value, $max_choices);

    //Add a track list element?
    $track_list = !empty($element['#track_list']) && $element['#track_list'];
    if ( $track_list ) {
      $element[] = array(
        '#type' => 'checkbox_tree_track_list',
        '#max_choices' => $max_choices,
      );
    }
  } 

  return $element;
}

/**
 * Returns HTML for a checkbox_tree form element.
 *
 * @param $variables
 *   An associative array containing:
 *   - element: An associative array containing the properties of the element.
 *
 * @ingroup themeable
 */
function theme_checkbox_tree($variables) {
  $element = $variables['element'];
  $element['#children'] = drupal_render_children($element);
  
  $attributes = array();
  if (isset($element['#id'])) {
    $attributes['id'] = $element['#id'];
  }
  $attributes['class'][] = 'term-reference-tree';
  
  if (form_get_error($element)) {
    $attributes['class'][] = 'error';
  }
  
  if (!empty($element['#required'])) {
    $attributes['class'][] = 'required';
  }

  if (array_key_exists('#start_minimized', $element) && $element['#start_minimized']) {
    $attributes['class'][] = "term-reference-tree-start-minimized";
  }

  $add_track_list = FALSE;
  if (array_key_exists('#track_list', $element) && $element['#track_list']) {
    $attributes['class'][] = "term-reference-tree-track-list-shown";
    $add_track_list = TRUE;
  }

  if (!empty($element['#attributes']['class'])) {
    $attributes['class'] = array_merge($attributes['class'], $element['#attributes']['class']);
  }
  return 
      '<div' . drupal_attributes($attributes) . '>' 
    . (!empty($element['#children']) ? $element['#children'] : '') 
    . '</div>';
}


/**
 * This function prints a list item with a checkbox and an unordered list
 * of all the elements inside it.
 */
function theme_checkbox_tree_level($variables) {
  $element = $variables['element'];
  $sm = '';
  if (array_key_exists('#start_minimized', $element) && $element['#start_minimized']) {
    $sm = " term-reference-tree-start-minimized";
  }

  $max_choices = 0;
  if (array_key_exists('#max_choices', $element)) {
    $max_choices = $element['#max_choices'];
  }

  $output = "<ul class='term-reference-tree-level$sm'>";
  $children = element_children($element);
  foreach($children as $child) {
    $output .= "<li>";
    $output .= drupal_render($element[$child]);
    $output .= "</li>";
  }

  $output .= "</ul>";

  return $output;
}

/**
 * This function prints a single item in the tree, followed by that item's children
 * (which may be another checkbox_tree_level).
 */
function theme_checkbox_tree_item($variables) {
  $element = $variables['element'];
  $children = element_children($element);
  $output = "";
  
  if (is_array($children) && count($children) > 1) {
    $output .= "<div class='term-reference-tree-button'></div>";

  }
  elseif (!$element['#leaves_only']) {
    $output .= "<div class='no-term-reference-tree-button'></div>";
  }

  foreach($children as $child) {
    $output .= drupal_render($element[$child]);
  }
  
  return $output;
}

/**
 * This function prints a label that cannot be selected.
 */
function theme_checkbox_tree_label($variables) {
  $element = $variables['element'];
  $output = "<div class='parent-term'>" . $element['#value'] . "</div>";
  return $output;
}

/**
 * Shows a list of items that have been checked.
 * The display happens on the client-side.
 * Use this function to theme the element's label,
 * and the "nothing selected" message.
 * 
 * @param $variables Variables available for theming. 
 */
function theme_checkbox_tree_track_list($variables) {
  //Should the label be singular or plural? Depends on cardinality of term field.
  $nothingselected = t('[Nothing selected]');
  $label = format_plural(
      $variables['element']['#max_choices'], 
      'Selected item (click the item to uncheck it)',
      'Selected items (click an item to uncheck it)'
  );
  $output = 
    '<div class="term-reference-track-list-container">
       <div class="term-reference-track-list-label">' . $label . '</div>
       <ul class="term-reference-tree-track-list"><li class="term_ref_tree_nothing_message">'.$nothingselected.'</li></ul>
     </div>';
     
  //Add the "Nothing selected" text. To style it, replace it with whatever you want.
  //Could do this with a file instead.
  drupal_add_js(
      'var termReferenceTreeNothingSelectedText = "' . $nothingselected . '";',
      'inline'
  );
  return $output;
}


/**
 * Implements hook_widget_field_form().
 */
function term_reference_tree_field_widget_form(&$form, &$form_state, $field, $instance, $langcode, $items, $delta, $element) {
  $settings = $instance['widget']['settings'];
  $voc = taxonomy_vocabulary_machine_name_load($field['settings']['allowed_values'][0]['vocabulary']);
  $path = drupal_get_path('module', 'term_reference_tree');
  $value_key = key($field['columns']);
  $type = $instance['widget']['type'];
  
  $default_value = array();
  foreach($items as $item) {
    $key = $item[$value_key];
    if ($key === 0) {
      $default_value[$key] = '0';
    }
    else {
      $default_value[$key] = $key;
    }
  }

  $multiple = $field['cardinality'] > 1 || $field['cardinality'] == FIELD_CARDINALITY_UNLIMITED;
  $properties = array();

  if (!array_key_exists('#value', $element))
    $element['#value'] = array();

  // A switch statement, in case we ever add more widgets to this module.
  switch($instance['widget']['type']) {
    case 'term_reference_tree':
      $element['#attached']['js'] = array($path . '/term_reference_tree.js');
      $element['#attached']['css'] = array($path . '/term_reference_tree.css');
      $element['#type'] = 'checkbox_tree';   
      $element['#default_value'] = $multiple ? $default_value : array(reset($default_value) => reset($default_value));
      $element['#max_choices'] = $field['cardinality'];
      $element['#start_minimized'] = $settings['start_minimized'];
      $element['#leaves_only'] = $settings['leaves_only'];
      $element['#filter_view'] = module_exists('views') ? $settings['filter_view'] : '';
      $element['#select_parents'] = $settings['select_parents'];
      $element['#track_list'] = $settings['track_list'];
      $element['#parent_tid'] = $field['settings']['allowed_values'][0]['parent'];
      $element['#vocabulary'] = $voc->vid;
      $element['#token_display'] = module_exists('token') ? $settings['token_display'] : '';
      break;
  }
  
  $element += array(
    '#value_key' => $value_key,
    '#element_validate' => array('_term_reference_tree_widget_validate'),
    '#properties' => $properties,
  );

  return $element;
}


/**
 * Validates the term reference tree widgets.
 * 
 * This function sets the value of the tree widgets into a form that Drupal
 * can understand, and also checks if the field is required and has been
 * left empty.
 * 
 * @param $element
 *   The element to be validated.
 * @param $form_state
 *   The state of the form.
 * 
 * @return
 *   The validated element.
 */
function _term_reference_tree_widget_validate(&$element, &$form_state) {
  $items = _term_reference_tree_flatten($element, $form_state);
  $value = array();

  if ($element['#max_choices'] != 1) {
    foreach($items as $child) {
      if (array_key_exists('#value', $child) && $child['#value'] !== 0) {
        array_push($value, array($element['#value_key'] => $child['#value']));

        // If the element is leaves only and select parents is on, then automatically
        // add all the parents of each selected value.
        if ($element['#select_parents'] && $element['#leaves_only']) {
          foreach($child['#parent_values'] as $parent_tid) {
            if (!in_array(array($element['#value_key'] => $parent_tid), $value)) {
              array_push($value, array($element['#value_key'] => $parent_tid));
            }
          }
        }
      } 
    }
  }
  else {
    // If it's a tree of radio buttons, they all have the same value, so we can just
    // grab the value of the first one.
    if (count($items) > 0) {
      $child = reset($items);
      if (array_key_exists('#value', $child) && $child['#value'] !== 0) {
        array_push($value, array($element['#value_key'] => $child['#value']));
      }       
    }
  }
  
  if ($element['#required'] && empty($value)) {
    form_error($element, t('!name field is required.', array('!name' => $element['#title'])));
  }

  form_set_value($element, $value, $form_state);
  return $element;
}

/**
 * Returns an array of allowed values defined by the given view.
 *
 * @param $filter
 *   A view, in the format VIEWNAME:DISPLAYNAME
 *
 * @return
 *   An array of term IDs (tid => true) returned by the view.
 */
function _term_reference_tree_get_allowed_values($filter) {
  $viewname = "";
  $displayname = "";
  $allowed = array();

  if (module_exists('views') && $filter != '') {
    list($viewname, $displayname) = explode(":", $filter);
    $view = views_get_view($viewname);
    if (is_object($view)) {
      if ($view->access($displayname)) {
        $view->execute_display($displayname);
        foreach($view->result as $item) {
          $allowed[$item->tid] = true;
        }
      }
      else {
        drupal_set_message("Cannot access view for term reference tree widget.", 'warning');
      }
    }
    else {
      drupal_set_message("Term reference tree: no view named '$viewname'", 'warning');
    }
  }

  return $allowed;
}



/**
 * Builds a single item in the term reference tree widget.
 *
 * This function returns an element with a checkbox for a single taxonomy term.
 * If that term has children, it appends checkbox_tree_level element that
 * contains the children.  It is meant to be called recursively when the widget
 * is built.
 *
 * @param $element
 *   The main checkbox_tree element.
 * @param $term
 *   A taxonomy term object.  $term->children should be an array of the term
 *   objects that are that term's children.
 * @param $form_state
 *   The form state.
 * @param $value
 *   The value of the element.
 * @param $max_choices
 *   The maximum number of allowed selections.
 *
 * @return
 *   A completed checkbox_tree_item element, which contains a checkbox and
 *   possibly a checkbox_tree_level element as well.
 */
function _term_reference_tree_build_item(&$element, &$term, &$form_state, &$value, $max_choices, $parent_tids) {
  $start_minimized = FALSE;
  if (array_key_exists('#start_minimized', $element)) {
    $start_minimized = $element['#start_minimized'];
  }

  $leaves_only = FALSE;
  if (array_key_exists('#leaves_only', $element)) {
    $leaves_only = $element['#leaves_only'];
  }
  
  $container = array(
    '#type' => 'checkbox_tree_item',
    '#max_choices' => $max_choices,
    '#start_minimized' => $start_minimized,
    '#leaves_only' => $leaves_only,
    '#term_name' => entity_label('taxonomy_term', $term),
  );

  if (!$element['#leaves_only'] || count($term->children) == 0) {
    $name = "edit-" . str_replace('_', '-', $element['#field_name']);
    $e = array(
      '#type' => ($max_choices == 1) ? 'radio' : 'checkbox',
      '#title' => entity_label('taxonomy_term', $term),
      '#on_value' => $term->tid,
      '#off_value' => 0,
      '#return_value' => $term->tid,
      '#parent_values' => $parent_tids,
      '#default_value' => isset($value[$term->tid]) ? $term->tid : NULL,
      '#attributes' => isset($element['#attributes']) ? $element['#attributes'] : NULL,
      '#ajax' => isset($element['#ajax']) ? $element['#ajax'] : NULL,
    );

    if ($element['#token_display'] != '' && module_exists('token')) {
      $t = taxonomy_term_load($term->tid);
      $e['#title'] = token_replace($element['#token_display'], array('term' => $t), array('clear' => TRUE));
    }

    if ($e['#type'] == 'radio') {
      $parents_for_id = array_merge($element['#parents'], array($term->tid));
      $e['#id'] = drupal_html_id('edit-' . implode('-', $parents_for_id));
      $e['#parents'] = $element['#parents'];
    }
  }
  else {
    $e = array(
      '#type' => 'checkbox_tree_label',
      '#value' => entity_label('taxonomy_term', $term),
    );
  }

  $container[$term->tid] = $e;
  

  if (property_exists($term, 'children') && count($term->children) > 0) {
    $parents = $parent_tids;
    $parents[] = $term->tid;
    $container[$term->tid . '-children'] = _term_reference_tree_build_level($element, $term, $form_state, $value, $max_choices, $parents);
  }

  return $container;
}

/**
 * Builds a level in the term reference tree widget.
 *
 * This function returns an element that has a number of checkbox_tree_item elements
 * as children.  It is meant to be called recursively when the widget is built.
 *
 * @param $element
 *   The main checkbox_tree element.
 * @param $term
 *   A taxonomy term object.  $term->children should be an array of the term
 *   objects that are that term's children.
 * @param $form_state
 *   The form state.
 * @param $value
 *   The value of the element.
 * @param $max_choices
 *   The maximum number of allowed selections.
 *
 * @return
 *   A completed checkbox_tree_level element.
 */
function _term_reference_tree_build_level(&$element, &$term, &$form_state, &$value, $max_choices, $parent_tids = array()) {
  $start_minimized = FALSE;
  if (array_key_exists('#start_minimized', $element)) {
    $start_minimized = $element['#start_minimized'];
  }

  $leaves_only = FALSE;
  if (array_key_exists('#leaves_only', $element)) {
    $leaves_only = $element['#leaves_only'];
  }
  
  $container = array(
    '#type' => 'checkbox_tree_level',
    '#max_choices' => $max_choices,
    '#start_minimized' => $start_minimized,
    '#leaves_only' => $leaves_only,
  );

  foreach($term->children as $t) {
    $container[$t->tid] = _term_reference_tree_build_item($element, $t, $form_state, $value, $max_choices, $parent_tids);
  }
  
  return $container;
}