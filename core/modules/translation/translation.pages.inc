<?php
/**
 * @file
 * User page callbacks for the Translation module.
 */

/**
 * Page callback: Displays a list of a node's translations.
 *
 * @param Node $node
 *   A node entity.
 *
 * @return
 *   A render array for a page containing a list of content.
 *
 * @see translation_menu()
 */
function translation_node_overview(Node $node) {
  include_once BACKDROP_ROOT . '/core/includes/language.inc';

  if ($node->tnid) {
    // Already part of a set, grab that set.
    $tnid = $node->tnid;
    $translations = translation_node_get_translations($node->tnid);
  }
  else {
    // We have no translation source nid, this could be a new set, emulate that.
    $tnid = $node->nid;
    $translations = array($node->langcode => $node);
  }

  $type = config_get('translation.settings', 'language_type');
  $header = array(t('Language'), t('Title'), t('Status'), t('Operations'));
  $rows = array();

  foreach (language_list(TRUE, TRUE) as $langcode => $language_name) {
    $options = array();
    if (isset($translations[$langcode])) {
      // Existing translation in the translation set: display status.
      // We load the full node to check whether the user can edit it.
      $translation_node = node_load($translations[$langcode]->nid);
      $path = 'node/' . $translation_node->nid;
      $links = language_negotiation_get_switch_links($type, $path);
      $title = empty($links->links[$langcode]['href']) ? l($translation_node->title, $path) : l($translation_node->title, $links->links[$langcode]['href'], $links->links[$langcode]);
      if (node_access('update', $translation_node)) {
        $path = 'node/' . $translation_node->nid . '/edit';
        $links = language_negotiation_get_switch_links($type, $path);
        if (!empty($links->links[$langcode]['href'])) {
          $options['edit'] = array(
            'title' => t('Edit'),
          ) + $links->links[$langcode];
        }
      }
      $status = $translation_node->status ? t('Published') : t('Not published');
      $status .= $translation_node->translate ? ' - <span class="marker">' . t('outdated') . '</span>' : '';
      if ($translation_node->nid == $tnid) {
        $language_name = t('<strong>@language_name</strong> (source)', array('@language_name' => $language_name));
      }
    }
    else {
      // No such translation in the set yet: help user to create it.
      $title = t('N/A');
      if (node_access('create', $node)) {
        $path = 'node/add/' . str_replace('_', '-', $node->type);
        $links = language_negotiation_get_switch_links($type, $path);
        $query = array('query' => array('translation' => $node->nid, 'target' => $langcode));
        if (!empty($links->links[$langcode]['href'])) {
          $options['add'] = array(
            'title' => t('Add translation'),
          ) + $links->links[$langcode];
          $options['add'] = array_merge_recursive($options['add'], $query);
        }
      }
      $status = t('Not translated');
    }
    $row = array();
    $row[] = $language_name;
    $row[] = $title;
    $row[] = $status;
    $row[] = array(
      'data' => array(
        '#type' => 'operations',
        '#links' => $options,
      ),
    );
    $rows[] = $row;
  }

  backdrop_set_title(t('Translations of %title', array('%title' => $node->title)), PASS_THROUGH);

  $build['translation_node_overview'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
  );

  return $build;
}
