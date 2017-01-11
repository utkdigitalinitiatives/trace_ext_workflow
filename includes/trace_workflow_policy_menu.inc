<?php
/**
 * @file
 * Management form for the Islandora Simple Workflow module.
 */

/**
 * Builds the management form for the Islandora Simple Workflow module.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 *
 * @return array
 *   An array containing the form to be rendered.
 */
function trace_workflow_policy_form($form, $form_state) {
  module_load_include('inc', 'trace_workflow_policy', 'includes/trace_workflow_policy_dao');

  if (isset($form_state['show_accept'])) {
    $form['confirm_message'] = array(
      '#type' => 'item',
      '#markup' => format_plural(count($form_state['pids_to_accept']),
      'Are you sure you want to accept the 1 object?',
      'Are you sure you want to accept the @count objects?'),
    );
    $form['confirm_accept'] = array(
      '#type' => 'submit',
      '#value' => t('Confirm'),
      '#weight' => 2,
      '#submit' => array('trace_workflow_policy_confirm_accept'),
    );
    $form['cancel_accept'] = array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
      '#weight' => 3,
    );
  }
  else  {
    $submitted_records = trace_workflow_policy_select_submitted_records();
    $rows = array();
    if (! is_null($submitted_records)) {
        foreach ($submitted_records as $submitted_pid => $submitted_label) {
          if (can_islandora_simple_workflow_object_read_user_role($submitted_pid)) {
            $rows[$submitted_pid] = array(l($submitted_label, "islandora/object/$submitted_pid"));
          }
        }
    }
    $form['management_table'] = array(
      '#type' => 'tableselect',
      '#header' => array(t('Object')),
      '#options' => $rows,
      '#attributes' => array(),
      '#empty' => t('No submitted objects were found.'),
    );
    if (!empty($rows)) {
      $form['submit_accepted'] = array(
        '#type' => 'submit',
        '#name' => 'trace-workflow-policy-accept-selected',
        '#validate' => array('trace_workflow_policy_selected_validate'),
        '#submit' => array('trace_workflow_policy_accept'),
        '#value' => t('Accepted Selected'),
      );
      $form['accept_all'] = array(
        '#type' => 'submit',
        '#name' => 'trace-workflow-policy-accept-all',
        '#submit' => array('trace_workflow_policy_accept'),
        '#value' => t('Accept All'),
      );
    }
  }
  return $form;
}

/**
 * Validation for the Islandora Simple Workflow management form.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function trace_workflow_policy_selected_validate($form, $form_state) {
  $selected = array_filter($form_state['values']['management_table']);
  if (empty($selected)) {
    form_error($form['management_table'], t('At least one object must be selected to publish!'));
  }
}

/**
 * Submit handler for the publish buttons in the workflow management form.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function trace_workflow_policy_accept(&$form, &$form_state) {
  if ($form_state['triggering_element']['#name'] == 'islandora-simple-workflow-submit-selected') {
    $selected = array_keys(array_filter($form_state['values']['management_table']));
  }
  else {
    $selected = array_keys($form_state['values']['management_table']);
  }
  $form_state['pids_to_accept'] = $selected;
  // Rebuild to show the confirm form.
  $form_state['rebuild'] = TRUE;
  $form_state['show_accept'] = TRUE;
}

/**
 * Submit handler for the workflow management confirm form.
 *
 * @param array $form
 *   An array representing a form within Drupal.
 * @param array $form_state
 *   An array containing the Drupal form state.
 */
function trace_workflow_policy_confirm_accept($form, &$form_state) {
  module_load_include('inc', 'trace_workflow_policy', 'includes/trace_workflow_policy_dao');

  $batch = trace_workflow_policy_accept_create_batch($form_state['pids_to_accept']);
  batch_set($batch);
}

