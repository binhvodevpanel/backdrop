<?php
/**
 * @file
 * Definition of views_handler_field_term_link_edit.
 */

/**
 * Field handler to present a term edit link.
 *
 * @ingroup views_field_handlers
 */
class views_handler_field_term_link_edit extends views_handler_field {
  function construct() {
    parent::construct();
    $this->additional_fields['tid'] = 'tid';
    $this->additional_fields['vocabulary'] = 'vocabulary';
  }

  function option_definition() {
    $options = parent::option_definition();

    $options['text'] = array('default' => '', 'translatable' => TRUE);

    return $options;
  }

  function options_form(&$form, &$form_state) {
    $form['text'] = array(
      '#type' => 'textfield',
      '#title' => t('Text to display'),
      '#default_value' => $this->options['text'],
    );
    parent::options_form($form, $form_state);
  }

  function query() {
    $this->ensure_my_table();
    $this->add_additional_fields();
  }

  function render($values) {
    // Check there is an actual value, as on a relationship there may not be.
    if ($tid = $this->get_value($values, 'tid')) {
      // Mock a term object for taxonomy_term_access().
      $term = new TaxonomyTerm(array(
        'tid' => $tid,
        'vocabulary' => $this->get_value($values, 'vocabulary')
      ));
      if (taxonomy_term_access('update', $term)) {
        $text = !empty($this->options['text']) ? $this->options['text'] : t('edit');
        $tid = $this->get_value($values, 'tid');
        return l($text, 'taxonomy/term/'. $tid . '/edit', array('query' => backdrop_get_destination()));
      }
    }
  }
}
