<?php
/**
 * @file
 * Handles the server side AJAX interactions of Views.
 */

/**
 * @defgroup ajax Views AJAX library
 * @{
 * Handles the server side AJAX interactions of Views.
 */

/**
 * Menu callback to load a view via AJAX.
 */
function views_ajax() {
  if (isset($_REQUEST['view_name']) && isset($_REQUEST['view_display_id'])) {
    $name = $_REQUEST['view_name'];
    $display_id = $_REQUEST['view_display_id'];
    $args = isset($_REQUEST['view_args']) && $_REQUEST['view_args'] !== '' ? explode('/', $_REQUEST['view_args']) : array();
    $path = isset($_REQUEST['view_path']) ? rawurldecode($_REQUEST['view_path']) : NULL;
    $dom_id = isset($_REQUEST['view_dom_id']) ? preg_replace('/[^a-zA-Z0-9_-]+/', '-', $_REQUEST['view_dom_id']) : NULL;
    $pager_element = isset($_REQUEST['pager_element']) ? intval($_REQUEST['pager_element']) : NULL;

    $commands = array();

    // Remove all of this stuff from $_GET so it doesn't end up in pagers and tablesort URLs.
    foreach (array('view_name', 'view_display_id', 'view_args', 'view_path', 'view_dom_id', 'pager_element', 'view_base_path', 'ajax_html_ids', 'ajax_page_state') as $key) {
      if (isset($_GET[$key])) {
        unset($_GET[$key]);
      }
      if (isset($_REQUEST[$key])) {
        unset($_REQUEST[$key]);
      }
      if (isset($_POST[$key])) {
        unset($_POST[$key]);
      }
    }

    // Load the view.
    $view = views_get_view($name);
    if ($view && $view->access($display_id) && $view->set_display($display_id) && $view->display_handler->use_ajax()) {
      // Fix 'q' for paging.
      if (!empty($path)) {
        $_GET['q'] = $path;
      }

      // Add all $_POST data, because AJAX is always a post and many things,
      // such as tablesorts, exposed filters and paging assume $_GET.
      $_GET = $_POST + $_GET;

      // Overwrite the destination.
      // @see backdrop_get_destination()
      $origin_destination = $path;
      $query = backdrop_http_build_query(backdrop_get_query_parameters());
      if ($query != '') {
        $origin_destination .= '?' . $query;
      }
      $destination = &backdrop_static('backdrop_get_destination');
      $destination = array('destination' => $origin_destination);

      // Override the display's pager_element with the one actually used.
      if (isset($pager_element)) {
        $commands[] = views_ajax_command_scroll_top('.view-dom-id-' . $dom_id);
        $view->display[$display_id]->handler->set_option('pager_element', $pager_element);
      }
      // Reuse the same DOM id so it matches that in Backdrop.settings.
      $view->dom_id = $dom_id;

      $commands[] = ajax_command_replace('.view-dom-id-' . $dom_id, $view->preview($display_id, $args));
    }
    backdrop_alter('views_ajax_data', $commands, $view);
    return array('#type' => 'ajax', '#commands' => $commands);
  }
}

/**
 * Creates a Backdrop AJAX 'viewsHilite' command.
 *
 * @param $selector
 *   The selector to highlight
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_hilite($selector) {
  return array(
    'command' => 'viewsHilite',
    'selector' => $selector,
  );
}

/**
 * Creates a Backdrop AJAX 'addTab' command.
 *
 * @param $id
 *   The DOM ID.
 * @param $title
 *   The title.
 * @param $body
 *   The body.
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_add_tab($id, $title, $body) {
  $command = array(
    'command' => 'viewsAddTab',
    'id' => $id,
    'title' => $title,
    'body' => $body,
  );
  return $command;
}

/**
 * Scroll to top of the current view.
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_scroll_top($selector) {
  $command = array(
    'command' => 'viewsScrollTop',
    'selector' => $selector,
  );
  return $command;
}

/**
 * Shows Save and Cancel buttons.
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_show_buttons() {
  $command = array(
    'command' => 'viewsShowButtons',
  );
  return $command;
}

/**
 * Trigger the Views live preview.
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_trigger_preview() {
  $command = array(
    'command' => 'viewsTriggerPreview',
  );
  return $command;
}

/**
 * Replace the page title.
 *
 * @return
 *   An array suitable for use with the ajax_render() function.
 */
function views_ajax_command_replace_title($title) {
  $command = array(
    'command' => 'viewsReplaceTitle',
    'title' => $title,
    'siteName' => config_get_translated('system.core', 'site_name'),
  );
  return $command;
}

/**
 * Return an AJAX error.
 */
function views_ajax_error($message) {
  $commands = array();
  $commands[] = ajax_command_open_modal_dialog(t('Error'), $message);
  return $commands;
}

/**
 * Wrapper around backdrop_build_form to handle some AJAX stuff automatically.
 * This makes some assumptions about the client.
 */
function views_ajax_form_wrapper($form_id, &$form_state) {
  // This won't override settings already in.
  $form_state += array(
    'rerender' => FALSE,
    'no_redirect' => !empty($form_state['ajax']),
    'no_cache' => TRUE,
    'build_info' => array(
      'args' => array(),
    ),
  );

  $form = backdrop_build_form($form_id, $form_state);
  $output = backdrop_render($form);

  // These forms have the title built in, so set the title here:
  if (empty($form_state['ajax']) && !empty($form_state['title'])) {
    backdrop_set_title($form_state['title']);
    backdrop_add_css(backdrop_get_path('module', 'views_ui') . '/css/views_ui.admin.css');
  }

  if (!empty($form_state['ajax']) && (empty($form_state['executed']) || !empty($form_state['rerender']))) {
    // If the form didn't execute and we're using ajax, build up a
    // Ajax command list to execute.
    $commands = array();

    $display = '';
    if ($messages = theme('status_messages')) {
      $display = '<div class="views-messages">' . $messages . '</div>';
    }
    $display .= $output;

    $title = empty($form_state['title']) ? '' : $form_state['title'];
    $options = array(
      'dialogClass' => 'views-ui-dialog',
    );
    $commands[] = ajax_command_open_modal_dialog($title, $display, $options);

    if (!empty($form_state['#section'])) {
      $commands[] = views_ajax_command_hilite('.' . backdrop_clean_css_identifier($form_state['#section']));
    }

    return $commands;
  }

  // These forms have the title built in, so set the title here:
  if (empty($form_state['ajax']) && !empty($form_state['title'])) {
    backdrop_set_title($form_state['title']);
  }

  return $output;
}


/**
 * Page callback for views user autocomplete
 */
function views_ajax_autocomplete_user($string = '') {
  // The user enters a comma-separated list of user name. We only autocomplete the last name.
  $array = backdrop_explode_tags($string);

  // Fetch last name
  $last_string = trim(array_pop($array));
  $matches = array();
  if ($last_string != '') {
    $prefix = count($array) ? implode(', ', $array) . ', ' : '';

    if (strpos('anonymous', strtolower($last_string)) !== FALSE) {
      $matches[$prefix . 'Anonymous'] = 'Anonymous';
    }

    $result = db_select('users', 'u')
      ->fields('u', array('uid', 'name'))
      ->condition('u.name', db_like($last_string) . '%', 'LIKE')
      ->range(0, 10)
      ->execute()
      ->fetchAllKeyed();

    foreach ($result as $account) {
      $n = $account;
      // Commas and quotes in terms are special cases, so encode 'em.
      if (strpos($account, ',') !== FALSE || strpos($account, '"') !== FALSE) {
        $n = '"' . str_replace('"', '""', $account) . '"';
      }
      $matches[$prefix . $n] = check_plain($account);
    }
  }

  backdrop_json_output($matches);
}

/**
 * Page callback for views taxonomy autocomplete.
 *
 * @param $vid
 *   The vocabulary id of the tags which should be returned.
 *
 * @param $tags_typed
 *   The typed string of the user.
 *
 * @see taxonomy_autocomplete()
 */
function views_ajax_autocomplete_taxonomy($vocabulary_name, $tags_typed = '') {
  // The user enters a comma-separated list of tags. We only autocomplete the last tag.
  $tags_typed = backdrop_explode_tags($tags_typed);
  $tag_last = backdrop_strtolower(array_pop($tags_typed));

  $matches = array();
  if ($tag_last != '') {

    $query = db_select('taxonomy_term_data', 't');
    $query->addTag('translatable');
    $query->addTag('taxonomy_term_access');

    // Do not select already entered terms.
    if (!empty($tags_typed)) {
      $query->condition('t.name', $tags_typed, 'NOT IN');
    }
    // Select rows that match by term name.
    $tags_return = $query
      ->fields('t', array('tid', 'name'))
      ->condition('t.vocabulary', $vocabulary_name)
      ->condition('t.name', '%' . db_like($tag_last) . '%', 'LIKE')
      ->range(0, 10)
      ->execute()
      ->fetchAllKeyed();

    $prefix = count($tags_typed) ? backdrop_implode_tags($tags_typed) . ', ' : '';

    $term_matches = array();
    foreach ($tags_return as $tid => $name) {
      $n = $name;
      // Term names containing commas or quotes must be wrapped in quotes.
      if (strpos($name, ',') !== FALSE || strpos($name, '"') !== FALSE) {
        $n = '"' . str_replace('"', '""', $name) . '"';
      }
      // Add term name to list of matches.
      $term_matches[$prefix . $n] = check_plain($name);
    }
  }

  backdrop_json_output($term_matches);
}

/**
 * @}
 */
