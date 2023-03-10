<?php
/**
 * @file
 * User page callbacks for the Comment module.
 */

/**
 * Form constructor for the comment reply form.
 *
 * There are several cases that have to be handled, including:
 *   - replies to comments
 *   - replies to nodes
 *   - attempts to reply to nodes that can no longer accept comments
 *   - respecting access permissions ('access comments', 'post comments', etc.)
 *
 * The node or comment that is being replied to must appear above the comment
 * form to provide the user context while authoring the comment.
 *
 * @param Node $node
 *   Every comment belongs to a node. This is that node.
 * @param $pid
 *   (optional) Some comments are replies to other comments. In those cases,
 *   $pid is the parent comment's comment ID. Defaults to NULL.
 *
 * @return array
 *   An associative array containing:
 *   - An array for rendering the node or parent comment.
 *     - comment_node: If the comment is a reply to the node.
 *     - comment_parent: If the comment is a reply to another comment.
 *   - comment_form: The comment form as a renderable array.
 */
function comment_reply(Node $node, $pid = NULL) {
  // Set the breadcrumb trail.
  backdrop_set_breadcrumb(array(l(t('Home'), NULL), l($node->title, 'node/' . $node->nid)));
  $op = isset($_POST['op']) ? $_POST['op'] : '';
  $build = array();

  // The user is previewing a comment prior to submitting it.
  if ($op == t('Preview')) {
    if (user_access('post comments')) {
      $comment = entity_create('comment', array('nid' => $node->nid, 'pid' => $pid));
      $build['comment_form'] = backdrop_get_form("comment_node_{$node->type}_form", $comment);
    }
    else {
      backdrop_set_message(t('You are not authorized to post comments.'), 'error');
      backdrop_goto("node/$node->nid");
    }
  }
  else {
    // $pid indicates that this is a reply to a comment.
    if ($pid) {
      if (user_access('access comments')) {
        // Load the comment whose cid = $pid
        $comment = comment_load($pid);
        if ($comment && $comment->status == COMMENT_PUBLISHED) {
          // If that comment exists, make sure that the current comment and the
          // parent comment both belong to the same parent node.
          if ($comment->nid != $node->nid) {
            // Attempting to reply to a comment not belonging to the current nid.
            backdrop_set_message(t('The comment you are replying to does not exist.'), 'error');
            backdrop_goto("node/$node->nid");
          }
          // Display the parent comment
          $comment->node_type = 'comment_node_' . $node->type;
          $comment->name = $comment->uid ? $comment->registered_name : $comment->name;
          $build['comment_parent'] = comment_view($comment, $node);
        }
        else {
          backdrop_set_message(t('The comment you are replying to does not exist.'), 'error');
          backdrop_goto("node/$node->nid");
        }
      }
      else {
        backdrop_set_message(t('You are not authorized to view comments.'), 'error');
        backdrop_goto("node/$node->nid");
      }
    }
    // This is the case where the comment is in response to a node. Display the node.
    elseif (user_access('access content')) {
      $build['comment_node'] = node_view($node);
    }

    // Should we show the reply box?
    if ($node->comment != COMMENT_NODE_OPEN) {
      backdrop_set_message(t("This discussion is closed: you can't post new comments."), 'error');
      backdrop_goto("node/$node->nid");
    }
    elseif (user_access('post comments')) {
      $comment = entity_create('comment', array('nid' => $node->nid, 'pid' => $pid));
      $build['comment_form'] = backdrop_get_form("comment_node_{$node->type}_form", $comment);
    }
    else {
      backdrop_set_message(t('You are not authorized to post comments.'), 'error');
      backdrop_goto("node/$node->nid");
    }
  }

  return $build;
}

/**
 * Page callback: Publishes the specified comment.
 *
 * @param $cid
 *   A comment identifier.
 *
 * @see comment_menu()
 */
function comment_approve($cid) {
  if (!isset($_GET['token']) || !backdrop_valid_token($_GET['token'], "comment/$cid/approve")) {
    return MENU_ACCESS_DENIED;
  }
  if ($comment = comment_load($cid)) {
    $comment->status = COMMENT_PUBLISHED;
    comment_save($comment);

    backdrop_set_message(t('Comment approved.'));
    backdrop_goto('node/' . $comment->nid);
  }
  return MENU_NOT_FOUND;
}
