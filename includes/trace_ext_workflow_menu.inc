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
function trace_ext_workflow_form($form, $form_state) {
    module_load_include('inc', 'trace_ext_workflow', 'includes/trace_ext_workflow_dao');
    module_load_include('inc', 'islandora_simple_workflow', 'includes/utilities');
    module_load_include('inc', 'islandora', 'includes/utilities');

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
      '#submit' => array('trace_ext_workflow_confirm_accept'),
    );
    $form['cancel_accept'] = array(
      '#type' => 'submit',
      '#value' => t('Cancel'),
      '#weight' => 3,
    );
  }
  else  {
    $submitted_records = trace_ext_workflow_select_submitted_records();
    $rows = array();
    if (! is_null($submitted_records)) {
        foreach ($submitted_records as $submitted_pid => $submitted_row) {
          if (can_trace_workflow_user_role_access_object($submitted_pid)) {
            $rows[$submitted_pid] = array(l($submitted_row['label'], "islandora/object/$submitted_pid"), l($submitted_row['user'], "users/" . $submitted_row['user']), $submitted_row['representative_body'], $submitted_row['modified_date']);
          }
        }
    }
    $form['management_table'] = array(
      '#type' => 'tableselect',
      '#header' => array(t('Object'), t('Owner'), t('ETD'), t('Date Modified')),
      '#options' => $rows,
      '#attributes' => array(),
      '#empty' => t('No submitted objects were found.'),
    );
    if (!empty($rows)) {
      $form['submit_accepted'] = array(
        '#type' => 'submit',
        '#name' => 'trace-workflow-policy-accept-selected',
        '#validate' => array('trace_ext_workflow_selected_validate'),
        '#submit' => array('trace_ext_workflow_accept'),
        '#value' => t('Accepted Selected'),
      );
      $form['accept_all'] = array(
        '#type' => 'submit',
        '#name' => 'trace-workflow-policy-accept-all',
        '#submit' => array('trace_ext_workflow_accept'),
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
function trace_ext_workflow_selected_validate($form, $form_state) {
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
function trace_ext_workflow_accept(&$form, &$form_state) {
  if ($form_state['triggering_element']['#name'] == 'trace-workflow-policy-accept-selected') {
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
function trace_ext_workflow_confirm_accept($form, &$form_state) {
  module_load_include('inc', 'trace_ext_workflow', 'includes/trace_ext_workflow_dao');

  $batch = trace_ext_workflow_accept_create_batch($form_state['pids_to_accept']);
  batch_set($batch);
}
