<?php

function trace_workflow_policy_select_submitted_records() {
    // get the pids from the database table that are still in a state
    // of 's' or submitted
        

    $query = db_select('trace_workflow_pids', 'w')
        ->fields('w', array('pid', 'label'))
        ->condition('w.state', 's')
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        $results = $query->fetchAllKeyed();
        return $results;
    } else {
        return NULL;
    }
}


/**
 * @file
 * Batch functions for the Islandora Simple Workflow module.
 */

/**
 * Constructs the batch that will go out and accept objects.
 *
 * @param array $pids
 *   The array of pids to be accepted.
 *
 * @return array
 *   An array detailing the batch that is about to be ran.
 */
function trace_workflow_policy_accept_create_batch($pids) {
  // Set up a batch operation.
  $batch = array(
    'operations' => array(
      array('trace_workflow_policy_accept_batch_operation', array($pids)),
    ),
    'title' => t('Accepting the selected objects...'),
    'init_message' => t('Preparing to accept objects.'),
    'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
    'error_message' => t('An error has occurred.'),
    'finished' => 'trace_workflow_policy_accept_batch_finished',
    'file' => drupal_get_path('module', 'trace_workflow_policy') . '/includes/trace_workflow_policy_dao.inc',
  );
  return $batch;
}

/**
 * Constructs and performs the accepting batch operation.
 *
 * @param array $pids
 *   An array of pids to be accepted.
 * @param array $context
 *   The context of the Drupal batch.
 */
function trace_workflow_policy_accept_batch_operation($pids, &$context) {
  module_load_include('inc', 'islandora', 'includes/utilities');
  if (empty($context['sandbox'])) {
    $context['sandbox'] = array();
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['pids'] = $pids;
    $context['sandbox']['total'] = count($pids);
    $context['results']['success'] = array();
  }

  while (!empty($context['sandbox']['pids'])) {
    
    $target_pid = array_pop($context['sandbox']['pids']);
    // add  a new policy to this pid
    $target_object = islandora_object_load($target_pid);
    if ($target_object) {
      trace_workflow_policy_object_policy_update($target_object);
    } else {
        watchdog('trace_workflow_policy', 'trace_workflow_policy_accept_batch_operation cannot load object  %p.', array('%p' => $target_object), WATCHDOG_ERROR);
        return;
    }
    
    // this is a poor design, it is costly to run for each pid
    // update the database table to reflect the new state of 'accepted' or 'a'
    trace_workflow_policy_update_accepted_record($target_pid);
    $context['message'] = t('Accepted @current of @total...', array(
        '@current' => $context['sandbox']['progress'],
        '@total' => $context['sandbox']['total'],
    ));

    $context['results']['success'][] = $target_pid;
    $context['sandbox']['progress']++;
  }
  $context['finished'] = ($context['sandbox']['total'] == 0) ? 1 : ($context['sandbox']['progress'] / $context['sandbox']['total']);
}

/**
 * Finished function for the simple workflow accept batch.
 *
 * @param bool $success
 *   Whether the batch was successful or not.
 * @param array $results
 *   An array containing the results of the batch operations.
 * @param array $operations
 *   The operations array that was used in the batch.
 */
function trace_workflow_policy_accept_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = format_plural(count($results['success']), 'One object accepted.', '@count objects accepted.');
  }
  else {
    $message = t('Finished with an error.');
  }
  drupal_set_message($message);
}


/**
 * Rules action callback; set a POLiCY datastream on an object and return.
 */
function trace_workflow_policy_object_policy_add(AbstractObject $object) {
    // determine the namespace of the object
    $pid = $object->id;
    $namespace = islandora_get_namespace($pid);
                                                                                                       
    // get the policy from the database that corresponds to the namespace
    $policy_xml = trace_workflow_policy_retrieve_namespace_submitted_policy($namespace);
    trace_workflow_policy_insert_submitted_record($object);
    // add the policy to the object
    return trace_workflow_policy_add_policy_datastream($object, $policy_xml);
}

/**
 * Rules action callback; set a POLiCY datastream on an object and return.
 */
function trace_workflow_policy_object_policy_update(AbstractObject $object) {
    // determine the namespace of the object
    $pid = $object->id;
    $namespace = islandora_get_namespace($pid);


    // get the policy from the database that corresponds to the namespace
    $policy_xml = trace_workflow_policy_retrieve_namespace_accepted_policy($namespace);
    // add the policy to the object
    
    return trace_workflow_policy_update_policy_datastream($object, $policy_xml);
}
function trace_workflow_policy_retrieve_namespace_submitted_policy($namespace) {

    // get the policy from the database that corresponds to the namespace

    $query = db_select('trace_workflow_namespace_policies', 't')
        ->fields('t', array('submitted_policy'))
        ->condition('t.namespace', $namespace)
        ->range(0, 1)
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        $result = $query->fetchObject();
        return $result->submitted_policy;
    } else {
        watchdog('trace_workflow_policy', 'trace_workflow_namespace_policies does not have policies for  %p.', array('%p' => $namespace), WATCHDOG_ERROR);
        return FALSE;
    }
}
function trace_workflow_policy_retrieve_namespace_accepted_policy($namespace) {

    // get the policy from the database that corresponds to the namespace

    $query = db_select('trace_workflow_namespace_policies', 't')
        ->fields('t', array('accepted_policy'))
        ->condition('t.namespace', $namespace)
        ->range(0, 1)
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        $result = $query->fetchObject();
        return $result->accepted_policy;
    } else {
        watchdog('trace_workflow_policy', 'trace_workflow_namespace_policies does not have policies for  %p.', array('%p' => $namespace), WATCHDOG_ERROR);
        return FALSE;
    }
}
function trace_workflow_policy_add_policy_datastream($object, $policy_xml) {
    $datastream = $object->getDatastream('POLICY');
    if (empty($datastream)) {
        $datastream = $object->constructDatastream('POLICY', 'm');
        $datastream->mimetype = 'application/xml';
        $datastream->label = 'Policy';

        $datastream->state = 'A';
        $datastream->content = $policy_xml;

        if (!$object->ingestDatastream($datastream)) {
            watchdog('trace_workflow_policy', 'trace_workflow_namespace_policies did not ingest the policy for %p.', array('%p' => $object->id), WATCHDOG_ERROR);
        }
    } else {
        watchdog('trace_workflow_policy', 'trace_workflow_namespace_policies already has a policy for %p.', array('%p' => $datastream->id), WATCHDOG_INFO);
    }
    return $datastream;
}
function trace_workflow_policy_update_policy_datastream($object, $policy_xml) {
    $pid = $object->id;
    $repository = $object->repository;

    return $repository->api->m->modifyDatastream($pid, 'POLICY', array('dsString' => $policy_xml));

}
function trace_workflow_policy_insert_submitted_record($object) {
    try {
        global $user;
        $nid = db_insert('trace_workflow_pids')->fields(array(
                'pid' => $object->id,
                'label' => $object->label,
                'state' => 's',
                'user' => $user->name,
            ))->execute();
    } catch (Exception $e) {
        \drupal_set_message(t('Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage())), 'error');
        \watchdog('trace_workflow_policy', 'Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage()), WATCHDOG_ERROR);
    }
}
function trace_workflow_policy_update_accepted_record($pid) {
    try {
    db_update('trace_workflow_pids') // Table name no longer needs {}
      ->fields(array(
        'state' => 'a',
      ))
      ->condition('pid', $pid)
      ->execute();
    } catch (Exception $e) {
        \drupal_set_message(t('Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage())), 'error');
        \watchdog('trace_workflow_policy', 'Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage()), WATCHDOG_ERROR);
    }
}