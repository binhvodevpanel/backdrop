<?php
/**
 * @file
 * Page callbacks for the Contact module.
 */

/**
 * Form constructor for the site-wide contact form.
 *
 * @see contact_menu()
 * @see contact_site_form_validate()
 * @see contact_site_form_submit()
 * @ingroup forms
 */
function contact_site_form($form, &$form_state) {
  global $user;

  // Check if flood control has been activated for sending emails.
  $config = config('contact.settings');
  $limit = $config->get('contact_threshold_limit');
  $window = $config->get('contact_threshold_window');
  if (!flood_is_allowed('contact', $limit, $window) && !user_access('administer contact forms')) {
    backdrop_set_message(t("You cannot send more than %limit messages in @interval. Try again later.", array('%limit' => $limit, '@interval' => format_interval($window))), 'error');
    return array();
  }

  // Get an array of the categories.
  $config_data = contact_config_data();
  $categories = $config_data['categories'];
  $default_category = $config_data['contact_default_category'];

  // If there are no categories, do not display the form.
  if (!$categories) {
    if (user_access('administer contact forms')) {
      backdrop_set_message(t('The contact form has not been configured. <a href="@add">Add one or more categories</a> to the form.', array('@add' => url('admin/structure/contact/add'))), 'error');
    }
    else {
      backdrop_not_found();
      backdrop_exit();
    }
  }

  $options = array();
  foreach ($categories as $cat) {
    $options[$cat['cid']] = $cat['category'];
  }

  // If there is more than one category available and no default category has
  // been selected, prepend a default placeholder value.
  if ($default_category == 0) {
    if (count($options) > 1) {
      $options = array(0 => t('- Please choose -')) + $options;
    }
    else {
      $default_category = key($categories);
    }
  }

  if (!$user->uid) {
    $form['#attached']['library'][] = array('system', 'jquery.cookie');
    $form['#attributes']['class'][] = 'user-info-from-cookie';
  }

  $form['#attributes']['class'][] = 'contact-form';
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Your name'),
    '#maxlength' => 255,
    '#default_value' => $user->uid ? user_format_name($user) : '',
    '#required' => TRUE,
  );
  $form['mail'] = array(
    '#type' => 'email',
    '#title' => t('Your email address'),
    '#default_value' => $user->uid ? $user->mail : '',
    '#required' => TRUE,
  );
  $phone = $config->get('phone');
  if (!empty($phone)) {
    $form['phone'] = array(
      '#type' => 'tel',
      '#title' => t('Your phone number'),
      '#required' => $phone == 'required',
    );
  }
  $form['subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#maxlength' => 255,
    '#required' => TRUE,
  );

  $form['cid'] = array(
    '#type' => 'select',
    '#title' => t('Category'),
    '#default_value' => $default_category,
    '#options' => $options,
    '#required' => TRUE,
    '#access' => count($options) > 1,
  );
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#required' => TRUE,
  );
  // We do not allow anonymous users to send themselves a copy
  // because it can be abused to spam people.
  $form['copy'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send yourself a copy.'),
    '#access' => $user->uid && contact_show_personal_copy_checkbox(),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send message'),
  );

  return $form;
}

/**
 * Form validation handler for contact_site_form().
 *
 * @see contact_site_form_submit()
 */
function contact_site_form_validate($form, &$form_state) {
  if (!$form_state['values']['cid']) {
    form_set_error('cid', t('You must select a valid category.'));
  }
}

/**
 * Form submission handler for contact_site_form().
 *
 * @see contact_site_form_validate()
 */
function contact_site_form_submit($form, &$form_state) {
  global $user, $language;

  $values = $form_state['values'];
  $values['sender'] = clone $user;
  $values['sender']->name = $values['name'];
  $values['sender']->mail = $values['mail'];
  $values['category'] = contact_load($values['cid']);

  // Save the anonymous user information to a cookie for reuse.
  if (!$user->uid) {
    user_cookie_save(array_intersect_key($values, array_flip(array('name', 'mail'))));
  }

  // Get the to and from email addresses.
  $to = $values['category']['recipients'];
  $from = $values['sender']->mail;

  // Send the email to the recipients using the site default language.
  backdrop_mail('contact', 'page_mail', $to, language_default(), $values, $from);

  // If the user requests it, send a copy using the current language.
  if ($values['copy']) {
    backdrop_mail('contact', 'page_copy', $from, $language, $values, $from);
  }

  // Send an auto-reply if necessary using the current language.
  if ($values['category']['reply']) {
    backdrop_mail('contact', 'page_autoreply', $from, $language, $values, $to);
  }

  $config = config('contact.settings');
  flood_register_event('contact', $config->get('contact_threshold_window'));
  watchdog('mail', '%sender-name (@sender-from) sent an email regarding %category.',
    array(
      '%sender-name' => $values['name'],
      '@sender-from' => $from,
      '%category' => $values['category']['category'],
    ));

  // Jump to home page rather than back to contact page to avoid
  // contradictory messages if flood control has been activated.
  backdrop_set_message(t('Your message has been sent.'));
  $form_state['redirect'] = '';
}

/**
 * Form constructor for the personal contact form.
 *
 * @see contact_menu()
 * @see contact_personal_form_validate()
 * @see contact_personal_form_submit()
 * @ingroup forms
 */
function contact_personal_form($form, &$form_state, $recipient) {
  global $user;

  // Check if flood control has been activated for sending emails.
  $config = config('contact.settings');
  $limit = $config->get('contact_threshold_limit');
  $window = $config->get('contact_threshold_window');
  if (!flood_is_allowed('contact', $limit, $window) && !user_access('administer contact forms') && !user_access('administer users')) {
    backdrop_set_message(t("You cannot send more than %limit messages in @interval. Try again later.", array('%limit' => $limit, '@interval' => format_interval($window))), 'error');
    return array();
  }

  backdrop_set_title(t('Contact @username', array('@username' => user_format_name($recipient))), PASS_THROUGH);

  if (!$user->uid) {
    $form['#attached']['library'][] = array('system', 'jquery.cookie');
    $form['#attributes']['class'][] = 'user-info-from-cookie';
  }

  $form['#attributes']['class'][] = 'contact-form';
  $form['recipient'] = array(
    '#type' => 'value',
    '#value' => $recipient,
  );
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Your name'),
    '#maxlength' => 255,
    '#default_value' => $user->uid ? user_format_name($user) : '',
    '#required' => TRUE,
  );
  $form['mail'] = array(
    '#type' => 'email',
    '#title' => t('Your email address'),
    '#default_value' => $user->uid ? $user->mail : '',
    '#required' => TRUE,
  );
  $phone = $config->get('phone');
  if (!empty($phone)) {
    $form['phone'] = array(
      '#type' => 'tel',
      '#title' => t('Your phone number'),
      '#required' => $phone == 'required',
    );
  }
  $form['to'] = array(
    '#type' => 'item',
    '#title' => t('To'),
    '#markup' => theme('username', array('account' => $recipient)),
  );
  $form['subject'] = array(
    '#type' => 'textfield',
    '#title' => t('Subject'),
    '#maxlength' => 50,
    '#required' => TRUE,
  );
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#rows' => 15,
    '#required' => TRUE,
  );
  // We do not allow anonymous users to send themselves a copy
  // because it can be abused to spam people.
  $form['copy'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send yourself a copy.'),
    '#access' => $user->uid && contact_show_personal_copy_checkbox(),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Send message'),
  );
  return $form;
}

/**
 * Form submission handler for contact_personal_form().
 *
 * @see contact_personal_form_validate()
 */
function contact_personal_form_submit($form, &$form_state) {
  global $user, $language;

  $values = $form_state['values'];
  $values['sender'] = clone $user;
  $values['sender']->name = $values['name'];
  $values['sender']->mail = $values['mail'];

  // Save the anonymous user information to a cookie for reuse.
  if (!$user->uid) {
    user_cookie_save(array_intersect_key($values, array_flip(array('name', 'mail'))));
  }

  // Get the to and from email addresses.
  $to = $values['recipient']->mail;
  $from = $values['sender']->mail;

  // Send the email in the requested user language.
  backdrop_mail('contact', 'user_mail', $to, user_preferred_language($values['recipient']), $values, $from);

  // Send a copy if requested, using current page language.
  if ($values['copy']) {
    backdrop_mail('contact', 'user_copy', $from, $language, $values, $from);
  }

  $config = config('contact.settings');
  flood_register_event('contact', $config->get('contact_threshold_window'));
  watchdog('mail', '%sender-name (@sender-from) sent %recipient-name an email.',
    array(
      '%sender-name' => $values['name'],
      '@sender-from' => $from,
      '%recipient-name' => $values['recipient']->name,
    ));

  // Jump to the contacted user's profile page.
  backdrop_set_message(t('Your message has been sent.'));
  $form_state['redirect'] = user_access('access user profiles') ? 'user/' . $values['recipient']->uid : '';
}
