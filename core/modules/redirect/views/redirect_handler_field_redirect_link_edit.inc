<?php
/**
 * @file
 * Redirect field handler for edit links.
 */

class redirect_handler_field_redirect_link_edit extends views_handler_field {
  function construct() {
    parent::construct();
    $this->additional_fields['rid'] = 'rid';
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['text'] = array('default' => '', 'translatable' => TRUE);
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $form['text'] = array(
      '#type' => 'textfield',
      '#title' => t('Text to display'),
      '#default_value' => $this->options['text'],
    );
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    $rid = $values->{$this->aliases['rid']};
    $output = '';
    if (($redirect = redirect_load($rid)) && redirect_access('update', $redirect)) {
      $text = !empty($this->options['text']) ? $this->options['text'] : t('Edit');
      $output = l($text, "admin/config/urls/redirect/edit/" . $rid, array('query' => backdrop_get_destination()));
    }
    return $output;
  }
}
