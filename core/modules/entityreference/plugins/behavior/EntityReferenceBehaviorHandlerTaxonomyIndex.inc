<?php

/**
 * @file
 * Plugin class for the taxonomy behavior.
 */

/**
 * Extends an entityreference field to maintain its references to taxonomy terms
 * in the {taxonomy_index} table.
 *
 * Note, unlike entityPostInsert() and entityPostUpdate(), entityDelete()
 * is not needed as cleanup is performed by taxonomy module in
 * taxonomy_delete_node_index().
 */
class EntityReferenceBehaviorHandlerTaxonomyIndex extends EntityReferenceBehaviorHandler {

  /**
   * Overrides EntityReferenceBehaviorHandler::access().
   *
   * Ensure that it is only enabled for ER instances on nodes targeting
   * terms, and the core setting to maintain index is enabled.
   */
  public function access($field, $instance) {
    if ($instance['entity_type'] != 'node' || $field['settings']['target_type'] != 'taxonomy_term') {
      return;
    }

    if ($field['storage']['type'] !== 'field_sql_storage') {
      // Field doesn't use SQL storage.
      return;
    }

    return config_get('taxonomy.settings', 'maintain_index_table');
  }

  /**
   * Overrides EntityReferenceBehaviorHandler::entityPostInsert().
   *
   * Runs after hook_node_insert() used by taxonomy module.
   */
  public function entityPostInsert($entity_type, $entity, $field, $instance) {
    if ($entity_type != 'node') {
      return;
    }

    $this->buildNodeIndex($entity);
  }

  /**
   * Overrides EntityReferenceBehaviorHandler::entityPostUpdate().
   *
   * Runs after hook_node_update() used by taxonomy module.
   */
  public function entityPostUpdate($entity_type, $entity, $field, $instance) {
    if ($entity_type != 'node') {
      return;
    }

    $this->buildNodeIndex($entity);
  }

  /**
   * Builds and inserts taxonomy index entries for a given node.
   *
   * The index lists all terms that are related to a given node entity, and is
   * therefore maintained at the entity level.
   *
   * @param $node
   *   The node object.
   *
   * @see taxonomy_build_node_index()
   */
  protected function buildNodeIndex($node) {
    // Maintain a denormalized table of term/node relationships, containing
    // only data for current, published nodes.
    $status = NULL;
    if (config_get('taxonomy.settings', 'maintain_index_table')) {
      // If a node property is not set in the node object when node_save() is
      // called, the old value from $node->original is used.
      if (!empty($node->original)) {
        $status = (int)(!empty($node->status) || (!isset($node->status) && !empty($node->original->status)));
        $sticky = (int)(!empty($node->sticky) || (!isset($node->sticky) && !empty($node->original->sticky)));
      }
      else {
        $status = (int)(!empty($node->status));
        $sticky = (int)(!empty($node->sticky));
      }
    }
    // Only maintain the taxonomy index for published nodes.
    if ($status) {
      // Collect a unique list of all the term IDs from all node fields.
      $tid_all = array();
      foreach (field_info_instances('node', $node->type) as $instance) {
        $field_name = $instance['field_name'];
        $field = field_info_field($field_name);
        if (!empty($field['settings']['target_type']) && $field['settings']['target_type'] == 'taxonomy_term' && $field['storage']['type'] == 'field_sql_storage') {
          // If a field value is not set in the node object when node_save() is
          // called, the old value from $node->original is used.
          if (isset($node->{$field_name})) {
            $items = $node->{$field_name};
          }
          elseif (isset($node->original->{$field_name})) {
            $items = $node->original->{$field_name};
          }
          else {
            continue;
          }
          foreach (field_available_languages('node', $field) as $langcode) {
            if (!empty($items[$langcode])) {
              foreach ($items[$langcode] as $item) {
                $tid_all[$item['target_id']] = $item['target_id'];
              }
            }
          }
        }

        // Re-calculate the terms added in taxonomy_build_node_index() so
        // database queries can be optimized.
        $original_tid_all = array();
        if ($field['module'] == 'taxonomy' && $field['storage']['type'] == 'field_sql_storage') {
          // If a field value is not set in the node object when node_save() is
          // called, the old value from $node->original is used.
          if (isset($node->{$field_name})) {
            $items = $node->{$field_name};
          }
          elseif (isset($node->original->{$field_name})) {
            $items = $node->original->{$field_name};
          }
          else {
            continue;
          }
          foreach (field_available_languages('node', $field) as $langcode) {
            if (!empty($items[$langcode])) {
              foreach ($items[$langcode] as $item) {
                $original_tid_all[$item['tid']] = $item['tid'];
              }
            }
          }
        }
      }

      // Insert index entries for all the node's terms, that were not
      // already inserted in taxonomy_build_node_index().
      $tid_all = array_diff($tid_all, $original_tid_all);

      // Insert index entries for all the node's terms, preventing duplicates.
      if (!empty($tid_all)) {
        foreach ($tid_all as $tid) {
          $row = array(
            'nid' => $node->nid,
            'tid' => $tid,
            'sticky' => $sticky,
            'created' => $node->created,
          );
          $query = db_merge('taxonomy_index')
            ->key($row)
            ->fields($row);
          $query->execute();
        }
      }
    }
  }

  /**
   * Overrides EntityReferenceBehaviorHandler::settingsForm().
   */
  public function settingsForm($field, $instance) {
    $form = array();
    $target = $field['settings']['target_type'];
    if ($target != 'taxonomy_term') {
      $form['ti-on-terms'] = array(
        '#markup' => t('This behavior can only be set when the target type is taxonomy_term, but the target of this field is %target.', array('%target' => $target)),
      );
    }

    $entity_type = $instance['entity_type'];
    if ($entity_type != 'node') {
      $form['ti-on-nodes'] = array(
        '#markup' => t('This behavior can only be set when the entity type is node, but the entity type of this instance is %type.', array('%type' => $entity_type)),
      );
    }

    if (!config_get('taxonomy.settings', 'maintain_index_table')) {
      $form['ti-disabled'] = array(
        '#markup' => t('This core setting "maintain_index_table" is disabled.'),
      );
    }
    return $form;
  }
}
