<?php
/**
 * @file
 * Contains the RSS style plugin.
 */

/**
 * Default style plugin to render an RSS feed.
 *
 * @ingroup views_style_plugins
 */
class views_plugin_style_rss extends views_plugin_style {
  function attach_to($display_id, $path, $title) {
    $display = $this->view->display[$display_id]->handler;
    $url_options = array();
    $input = $this->view->get_exposed_input();
    if ($input) {
      $url_options['query'] = $input;
    }
    $url_options['absolute'] = TRUE;

    $url = url($this->view->get_url(NULL, $path), $url_options);
    if ($display->has_path()) {
      if (empty($this->preview)) {
        backdrop_add_feed($url, $title);
      }
    }
    else {
      if (empty($this->view->feed_icon)) {
        $this->view->feed_icon = '';
      }

      if ($this->options['icon']) {
        $this->view->feed_icon .= theme('feed_icon', array('url' => $url, 'title' => $title));
      }
      backdrop_add_html_head_link(array(
        'rel' => 'alternate',
        'type' => 'application/rss+xml',
        'title' => $title,
        'href' => $url
      ));
    }
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['description'] = array('default' => '', 'translatable' => TRUE);
    $options['icon'] = array('default' => TRUE, 'translatable' => TRUE);

    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    $form['description'] = array(
      '#type' => 'textfield',
      '#title' => t('RSS description'),
      '#default_value' => $this->options['description'],
      '#description' => t('This will appear in the RSS feed itself.'),
      '#maxlength' => 1024,
    );

    $form['icon'] = array(
      '#type' => 'checkbox',
      '#title' => t('Add RSS icon to displays'),
      '#default_value' => $this->options['icon'],
      '#description' => t('Show an RSS icon on the displays to which this RSS feed is attached.'),
    );
  }

  /**
   * Return an array of additional XHTML elements to add to the channel.
   *
   * @return
   *   An array that can be passed to format_xml_elements().
   */
  function get_channel_elements() {
    return array();
  }

  /**
   * Return an atom:link XHTML element to add to the channel, to comply with
   * the RSS 2.0 specification.
   *
   * @see http://validator.w3.org/feed/docs/warning/MissingAtomSelfLink.html
   *
   * @return
   *   An array that can be passed to format_xml_elements().
   */
  function get_channel_elements_atom_link() {
    $url_options = array('absolute' => TRUE);
    $input = $this->view->get_exposed_input();
    if ($input) {
      $url_options['query'] = $input;
    }
    $url = url($this->view->get_url(), $url_options);

    return array(
      array(
        'namespace' => array('xmlns:atom' => 'http://www.w3.org/2005/Atom'),
        'key' => 'atom:link',
        'attributes' => array(
          'href' => $url,
          'rel' => 'self',
          'type' => 'application/rss+xml',
        ),
      ),
    );
  }

  /**
   * Get RSS feed description.
   *
   * @return string
   *   The string containing the description with the tokens replaced.
   */
  function get_description() {
    $description = $this->options['description'];

    // Allow substitutions from the first row.
    $description = $this->tokenize_value($description, 0);

    return $description;
  }

  function render() {
    if (empty($this->row_plugin)) {
      watchdog('views', 'views_plugin_style_default: Missing row plugin');
      return;
    }
    $rows = '';

    // This will be filled in by the row plugin, and will be used later on in
    // the theme process.
    $this->namespaces = array(
      'xmlns:dc' => 'http://purl.org/dc/elements/1.1/',
      'xmlns:atom' => 'http://www.w3.org/2005/Atom',
    );

    // Fetch any additional elements for the channel and merge in their
    // namespaces.
    $this->channel_elements = array_merge(
      $this->get_channel_elements(),
      $this->get_channel_elements_atom_link()
    );
    foreach ($this->channel_elements as $element) {
      if (isset($element['namespace'])) {
        $this->namespaces = array_merge($this->namespaces, $element['namespace']);
      }
    }

    foreach ($this->view->result as $row_index => $row) {
      $this->view->row_index = $row_index;
      $rows .= $this->row_plugin->render($row);
    }

    $output = theme($this->theme_functions(),
      array(
        'view' => $this->view,
        'options' => $this->options,
        'rows' => $rows
      ));
    unset($this->view->row_index);
    return $output;
  }
}
