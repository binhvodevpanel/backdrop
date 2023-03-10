<?php
/**
 * @file
 * Password callback file for the user module.
 */

/**
 * Get the password strength score required for an account.
 *
 * @param object $username
 *   Backdrop username.
 *
 * @return bool
 *   True or false.
 */
function user_password_reject_weak($username) {
  // We use username instead of uid because this also runs for anonymous
  // users who are creating an account.
  $account = user_load_by_name($username);
  $reject_weak = config_get('system.core', 'user_password_reject_weak');

  // Allow other modules to alter this value (e.g. based on role).
  backdrop_alter('user_password_reject_weak', $reject_weak, $account);

  return $reject_weak;
}

/**
 * Generates help text for core password constraints on forms.
 *
 * @see user_account_form()
 */
function _user_password_policy_help() {
  // Label the threshold for the password description.
  $strength_threshold = config_get('system.core', 'user_password_strength_threshold');

  if ($strength_threshold < USER_PASSWORD_STRENGTH_FAIR) {
    $threshold_label = array('weak' => t('weak'));
  }
  elseif ($strength_threshold < USER_PASSWORD_STRENGTH_GOOD) {
    $threshold_label = array('fair' => t('fair'));
  }
  elseif ($strength_threshold < USER_PASSWORD_STRENGTH_STRONG) {
    $threshold_label = array('good' => t('good'));
  }
  else {
    $threshold_label = array('strong' => t('excellent'));
  }

  $criteria = array();
  $criteria[] = t('The password cannot be the same as the username or email address.');
  $criteria[] = t('The password must be sufficiently long or complex.');

  $help = '<div class="password-strength-help-text ' . key($threshold_label) . '">' . t('The password strength indicator must be %threshold or better.', array('%threshold' => current($threshold_label)));
  $help .= ' ' . t('The following criteria must be met:') . '</div>' . theme('item_list', array('items' => $criteria, 'type' => 'ul'));

  return $help;
}

/**
 * Evaluate password strength
 * Adapted from https://github.com/dropbox/zxcvbn.
 *
 * @see Backdrop.evaluatePasswordStrength
 *
 * @param string $password
 * @param string $username
 * @param string $email
 *
 * @return int
 *   An integer representing password strength.
 */
function _user_password_evaluate_strength($password, $username, $email) {
  $strength = 0;

  $has_lowercase = preg_match('/[a-z]+/', $password);
  $has_uppercase = preg_match('/[A-Z]+/', $password);
  $has_numbers = preg_match('/[0-9]+/', $password);
  $has_punctuation = preg_match('/[^a-zA-Z0-9]+/', $password);

  // Calculate the number of unique character sets within a string.
  $cardinality = ($has_lowercase * 26) + ($has_uppercase * 26) + ($has_numbers * 10) + ($has_punctuation * 33);

  // Assign strength based on the level of entropy within the password, times
  // its length.
  $length = backdrop_strlen($password);
  $strength = (log($cardinality) / log(2)) * $length + 1;

  // Adjust the strength so that we hit our desired password length for each
  // threshold. As computers improve, the recommended minimum length increases.
  $strength = $strength * USER_PASSWORD_STRENGTH_MODIFIER;

  if (backdrop_strtolower($password) == backdrop_strtolower($username)) {
    $strength = 5;
  }
  if (backdrop_strtolower($password) == backdrop_strtolower($email)) {
    $strength = 5;
  }

  return $strength;
}
