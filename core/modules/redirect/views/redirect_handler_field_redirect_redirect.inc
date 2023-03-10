<?php
/**
 * @file
 * Redirect field handler for {redirect}.redirect.
 */

class redirect_handler_field_redirect_redirect extends views_handler_field {
  function construct() {
    parent::construct();
    $this->additional_fields['redirect'] = 'redirect';
    $this->additional_fields['redirect_options'] = 'redirect_options';
  }

  function option_definition() {
    $options = parent::option_definition();
    $options['text'] = array('default' => '', 'translatable' => TRUE);
    $options['absolute'] = array('default' => FALSE);
    $options['alter']['contains']['make_link']['default'] = TRUE;
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);

    // This field will never be empty
    $form['empty']['#access'] = FALSE;
    $form['empty_zero']['#access'] = FALSE;
    $form['hide_empty']['#access'] = FALSE;

    $form['alter']['make_link']['#description'] = t('If checked, this field will be made into a link.');
    $form['alter']['absolute']['#access'] = FALSE;
    $form['alter']['path']['#access'] = FALSE;

    $form['text'] = array(
      '#type' => 'textfield',
      '#title' => t('Text to display'),
      '#default_value' => $this->options['text'],
    );
    $form['absolute'] = array(
      '#type' => 'checkbox',
      '#title' => t('Use absolute link (begins with "http://")'),
      '#default_value' => $this->options['absolute'],
      '#description' => t('If you want to use this as in "output this field as link" in "link path", you have to enabled this option.'),
    );
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    $redirect = $values->{$this->aliases['redirect']};
    $redirect_options = unserialize($values->{$this->aliases['redirect_options']});
    $redirect_options['absolute'] = !empty($this->options['absolute']);

    $url = redirect_url($redirect, $redirect_options);
    $text = !empty($this->options['text']) ? $this->options['text'] : $url;

    if (!empty($this->options['alter']['make_link'])) {
      $this->options['alter']['path'] = $url;
      $this->options['alter']['absolute'] = $redirect_options['absolute'];
    }
    else {
      $text = check_plain($text);
    }

    return $text;
  }
}
