<?php
/**
 * @file
 * Definition of views_handler_filter_boolean_operator_string.
 */

/**
 * Simple filter to handle matching of boolean values.
 *
 * This handler checks to see if a string field is empty (equal to '') or not.
 * It is otherwise identical to the parent operator.
 *
 * Definition items:
 * - label: (REQUIRED) The label for the checkbox.
 *
 * @ingroup views_filter_handlers
 */
class views_handler_filter_boolean_operator_string extends views_handler_filter_boolean_operator {
  function query() {
    $this->ensure_my_table();
    $where = "$this->table_alias.$this->real_field ";

    if (empty($this->value)) {
      $where .= "= ''";
      if ($this->accept_null) {
        $where = '(' . $where . " OR $this->table_alias.$this->real_field IS NULL)";
      }
    }
    else {
      $where .= "<> ''";
    }
    $this->query->add_where_expression($this->options['group'], $where);
  }
}
