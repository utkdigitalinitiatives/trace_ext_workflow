<?php

function trace_ext_workflow_select_submitted_records()
{
    // get the pids from the database table that are still in a state
    // of 's' or submitted

    $query = db_select('trace_workflow_pids', 'w')
        ->fields('w', array('pid', 'label', 'user', 'modified_date'))
        ->condition('w.state', 's')
        ->orderBy('modified_date', 'DESC')
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        $results = $query->fetchAllAssoc('pid', PDO::FETCH_ASSOC);
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
function trace_ext_workflow_accept_create_batch($pids)
{
    // Set up a batch operation.
    $batch = array(
        'operations' => array(
            array('trace_ext_workflow_accept_batch_operation', array($pids)),
        ),
        'title' => t('Accepting the selected objects...'),
        'init_message' => t('Preparing to accept objects.'),
        'progress_message' => t('Time elapsed: @elapsed <br/>Estimated time remaining @estimate.'),
        'error_message' => t('An error has occurred.'),
        'finished' => 'trace_ext_workflow_accept_batch_finished',
        'file' => drupal_get_path('module', 'trace_ext_workflow') . '/includes/trace_ext_workflow_dao.inc',
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
function trace_ext_workflow_accept_batch_operation($pids, &$context)
{
    module_load_include('inc', 'islandora', 'includes/utilities');
    $reapply_embargo_dsids = array();
    $reapply_embargo_users = array();
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
        $xacml_xml = null;
        $policy_dom = null;
        $embargoed_policy = null;
        try {
            $embargoed_policy = new Embargoed_Policy($target_object);
        } catch (Exception $exception) {
            $embargoed_policy = null;
        }
        if (isset($embargoed_policy)) {

            trace_ext_workflow_accept_object_policy_update($target_object);
            if ($embargoed_policy->isPolicyEmbargoed()) {
                $xacml_string = $embargoed_policy->transformWithEmbargo($target_object['POLICY']->content);
                $target_object['POLICY']->content = $xacml_string;
            }

        } else {
            trace_ext_workflow_accept_object_policy_update($target_object);
        }

        // this is a poor design, it is costly to run for each pid
        // update the database table to reflect the new state of 'accepted' or 'a'
        trace_ext_workflow_update_accepted_record($target_pid);
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
function trace_ext_workflow_accept_batch_finished($success, $results, $operations)
{
    if ($success) {
        $message = format_plural(count($results['success']), 'One object accepted.', '@count objects accepted.');
    } else {
        $message = t('Finished with an error.');
    }
    drupal_set_message($message);
}


/**
 * Y
 * Rules action callback; set a POLiCY datastream on an object and return.
 */
function trace_ext_workflow_object_policy_add($object)
{
    // XXX failure here somwhere?
    // determine the namespace of the object
    $pid = $object->id;
    $namespace = islandora_get_namespace($pid);
    // check to see if the object needs the additional workflow of
    // a collection manager accepting a submission
    // rather than the object going straight to publishing workflow
    //
    trace_ext_workflow_insert_submitted_record($object);
    $policy_xml = trace_ext_workflow_retrieve_namespace_submitted_policy($namespace);
    if ($policy_xml) {
        // get the policy from the database that corresponds to the namespace
        // add the policy to the object
        return trace_ext_workflow_add_policy_datastream($object, $policy_xml);
    } else {
        return NULL;
    }
}

/**
 * Y
 * Rules action callback; set a POLiCY datastream on an object and return.
 */
function trace_ext_workflow_accept_object_policy_update($object)
{
    // determine the namespace of the object
    $pid = $object->id;
    $namespace = islandora_get_namespace($pid);

    // get the policy from the database that corresponds to the namespace
    $policy_xml = trace_ext_workflow_retrieve_namespace_accepted_policy($namespace);
    // add the policy to the object
    if ($policy_xml) {
        $object['POLICY']->content = $policy_xml;
    } else {
        return NULL;
    }

}

/*
 * Y
 */
function trace_ext_workflow_retrieve_namespace_submitted_policy($namespace)
{

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
        watchdog('trace_ext_workflow', 'trace_workflow_namespace_policies does not have policies for  %p.', array('%p' => $namespace), WATCHDOG_INFO);
        return FALSE;
    }
}

/*
 * Y
 */
function trace_ext_workflow_retrieve_namespace_accepted_policy($namespace)
{

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
        watchdog('trace_ext_workflow', 'trace_workflow_namespace_policies does not have policies for  %p.', array('%p' => $namespace), WATCHDOG_ERROR);
        return FALSE;
    }
}

/*
 * Y
 */
function trace_ext_workflow_add_policy_datastream($object, $policy_xml)
{
    $datastream = $object->getDatastream('POLICY');
    if (empty($datastream)) {
        $datastream = $object->constructDatastream('POLICY', 'm');
        $datastream->mimetype = 'application/xml';
        $datastream->label = 'Policy';

        $datastream->state = 'A';
        $datastream->content = $policy_xml;

        if (!$object->ingestDatastream($datastream)) {
            watchdog('trace_ext_workflow', 'trace_workflow_namespace_policies did not ingest the policy for %p.', array('%p' => $object->id), WATCHDOG_ERROR);
        }
    } else {
        watchdog('trace_ext_workflow', 'trace_workflow_namespace_policies already has a policy for %p.', array('%p' => $datastream->id), WATCHDOG_INFO);
    }
    return $datastream;
}

/*
 * Y
 */
function trace_ext_workflow_update_policy_datastream($object, $policy_xml)
{
    $object['POLICY']->content = $policy_xml;

}

/*
 * Y
 */
function trace_ext_workflow_insert_submitted_record($object)
{
    try {
        global $user;
        $nid = db_insert('trace_workflow_pids')->fields(array(
            'pid' => $object->id,
            'label' => $object->label,
            'state' => 's',
            'user' => $user->name,
            'datastream' => date('Y-m-d H:i:s'),
        ))->execute();
    } catch (Exception $e) {
        \drupal_set_message(t('Error inserting submitted into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage())), 'error');
        \watchdog('trace_ext_workflow', 'Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $object->id, '%e' => $e->getMessage()), WATCHDOG_ERROR);
    }
}

/*
 * Y
 */
function trace_ext_workflow_update_accepted_record($pid)
{
    try {
        $num_updated = db_update('trace_workflow_pids')// Table name no longer needs {}
        ->fields(array(
            'state' => 'a',
            'datastream' => date('Y-m-d H:i:s'),
        ))
            ->condition('pid', $pid)
            ->execute();
    } catch (Exception $e) {
        \drupal_set_message(t('Error updating accepted in trace_worflow_pids table for pid %p : %e.', array('%p' => $pid, '%e' => $e->getMessage())), 'error');
        \watchdog('trace_ext_workflow', 'Error inserting into trace_worflow_pids table for pid %p : %e.', array('%p' => $pid, '%e' => $e->getMessage()), WATCHDOG_ERROR);
    }
}
/*
 * Y
 */
function trace_ext_workflow_delete_record_from_workflow($pid)
{
    try {
        db_delete('trace_workflow_pids')// Table name no longer needs {}
            ->condition('pid', $pid)
            ->execute();
    } catch (Exception $e) {
        \drupal_set_message(t('Error deleting record in trace_worflow_pids table for pid %p : %e.', array('%p' => $pid, '%e' => $e->getMessage())), 'error');
        \watchdog('trace_ext_workflow', 'Error deleting record in trace_worflow_pids table for pid %p : %e.', array('%p' => $pid, '%e' => $e->getMessage()), WATCHDOG_ERROR);
    }
}
/*
 * Y
 */
function is_trace_workflow_object_accepted($pid)
{
    $db_and = db_and();
    $db_and->condition('w.state', 'a');
    $db_and->condition('w.pid', $pid);
    $query = db_select('trace_workflow_pids', 'w')
        ->fields('w', array('pid'))
        ->condition($db_and)
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        return TRUE;
    }
    return FALSE;

}

/*
 * Y
 */
function can_trace_workflow_user_role_access_object($pid)
{
    global $user;
    $user_roles_array = array_keys($user->roles);
    $namespace = islandora_get_namespace($pid);
    $query = db_select('trace_workflow_namespace_role', 'iwnr');
    $db_or = db_or();
    $db_or->condition('iwnr.namespace', $namespace);
    $db_or->condition('iwnr.namespace', 'ALL');
    $db_and = db_and();
    $db_and->condition('iwnr.rid', $user_roles_array, 'IN');
    $db_and->condition($db_or);
    $query->condition($db_and);
    $query->addExpression('COUNT(iwnr.namespace)', 'namespace_count');
    $query->groupBy('iwnr.namespace');
    $results = $query->execute();
    $num = $results->rowCount();
    if ($num > 0) {
        $namespace_count_record = $results->fetchObject();
        if ($namespace_count_record && ($namespace_count_record->namespace_count > 0)) {
            return TRUE;
        }
    }
    return FALSE;
}

/*
 * Y
 */
function trace_ext_workflow_retrieve_namespace_published_policy($namespace)
{

    // get the policy from the database that corresponds to the namespace

    $query = db_select('trace_workflow_namespace_policies', 't')
        ->fields('t', array('published_policy'))
        ->condition('t.namespace', $namespace)
        ->range(0, 1)
        ->execute();

    $num = $query->rowCount();
    if ($num > 0) {
        $result = $query->fetchObject();
        return $result->published_policy;
    } else {
        watchdog('islandora_simple_workflow', 'islandora_simple_workflow_retrieve_namespace_published_policy does not have policies for  %p.', array('%p' => $namespace), WATCHDOG_ERROR);
        return FALSE;
    }
}

function trace_ext_workflow_retrieve_published_policy_xml($object) {
    // determine the namespace of the object
    $pid = $object->id;
    $namespace = islandora_get_namespace($pid);

    $embargoed_policy = null;
    // get the policy from the database that corresponds to the namespace
    $policy_xml = trace_ext_workflow_retrieve_namespace_published_policy($namespace);

    if (isset($policy_xml)) {
        try {
            $embargoed_policy = new Embargoed_Policy($object);
        } catch (Exception $exception) {
            watchdog('trace_ext_workflow', 'trace_ext_workflow_retrieve_published_policy_xml cannot parse policies for  %p.', array('%p' => $pid), WATCHDOG_ERROR);
            $embargoed_policy = null;
        }
        if (isset($embargoed_policy) && $embargoed_policy->isPolicyEmbargoed()) {
            $policy_xml = $embargoed_policy->transformWithEmbargo($policy_xml);
        }
    }
    return $policy_xml;

}
function trace_ext_workflow_accept_policy($target_object) {
    $target_pid = $target_object->id;
    // add  a new policy to this pid
    $policy_xml = null;
    $embargoed_policy = null;
    try {
        $embargoed_policy = new Embargoed_Policy($target_object);
    } catch (Exception $exception) {
        watchdog('trace_ext_workflow', 'trace_ext_workflow_accept_policy cannot parse policies for  %p.', array('%p' => $target_pid), WATCHDOG_ERROR);
        $embargoed_policy = null;
    }

    $namespace = islandora_get_namespace($target_pid);
    $policy_xml = trace_ext_workflow_retrieve_namespace_accepted_policy($namespace);
    if (isset($embargoed_policy) && $embargoed_policy->isPolicyEmbargoed()) {
        $policy_xml = $embargoed_policy->transformWithEmbargo($policy_xml);

    }
    return $policy_xml;

}
/**
 * Y
 * Rules action callback; set a POLiCY datastream on an object and return.
 */
function trace_ext_workflow_object_update($object)
{
    // add the policy to the object
    $object['POLICY']->content = trace_ext_workflow_retrieve_published_policy_xml($object);

}

class Embargoed_Policy
{
    /**
     * Unsets public members.
     *
     * We only define the public members of the object for Doxygen, they aren't actually accessed or used,
     * and if they are not unset, they can cause problems after unserialization.
     */
    private $policy_dom;
    private $xpath;
    private $pid;
    private $reapply_embargo_dsids = array();
    private $reapply_embargo_users = array();

    public function __construct($target_object)
    {
        module_load_include('inc', 'islandora_scholar_embargo', 'includes/embargo_xacml_policy');

        if (($target_object instanceof AbstractObject) && isset($target_object["POLICY"])) {
            $this->pid = $target_object->id;
            // likely there is an embargo here, so the policy should be checked for the embargo rule (dsids and userids will need to be grabbed and reapplied.
            $xacml_xml = $target_object['POLICY']->content;
            $this->policy_dom = islandora_scholar_embargo_dom_document_load($xacml_xml);
            $this->xpath = new DOMXPath($this->policy_dom);
            $this->xpath->registerNamespace("p", "urn:oasis:names:tc:xacml:1.0:policy");

        } else {
            throw new Exception("Target object is not a FedoraObject");
        }
    }

    public function isPolicyEmbargoed()
    {
        if ((count($this->reapply_embargo_users) > 0)) {
            return true;
        }
        $domNodeList = $this->xpath->query("/p:Policy/p:Rule[@RuleId='embargo-dsid-deny']");

        if ($domNodeList->length == 0) {
            return false;
        }

        // the dsids list may be empty if the entire object is being embargoed
        $domNodeList = $this->xpath->query("/p:Policy/p:Rule[@RuleId='embargo-dsid-deny']/p:Target/p:Resources/p:Resource/p:ResourceMatch/p:ResourceAttributeDesignator[@AttributeId='urn:fedora:names:fedora:2.1:resource:datastream:id']/parent::node()/p:AttributeValue");
        if ($domNodeList->length == 0) {
            watchdog('trace_ext_workflow', 'trace_ext_workflow_accept_batch_operation cannot parse embargoed dsids from object  %p.', array('%p' => $this->pid), WATCHDOG_INFO);
        } else {
            foreach ($domNodeList as $node) {
                if (XML_ELEMENT_NODE == $node->nodeType) {
                    $this->reapply_embargo_dsids[] = $node->nodeValue;
                }
            }
        }

        $domNodeList = $this->xpath->query("/p:Policy/p:Rule[@RuleId='embargo-dsid-deny']/p:Condition/p:Apply/p:Apply/p:SubjectAttributeDesignator[@AttributeId='urn:fedora:names:fedora:2.1:subject:loginId']/parent::node()/p:Apply/p:AttributeValue");
        if ($domNodeList->length == 0) {
            watchdog('trace_ext_workflow', 'trace_ext_workflow_accept_batch_operation cannot parse embargoed users from object  %p.', array('%p' => $this->pid), WATCHDOG_ERROR);
            return false;
        } else {
            foreach ($domNodeList as $node) {
                if (XML_ELEMENT_NODE == $node->nodeType) {
                    $this->reapply_embargo_users[] = $node->textContent;
                }
            }
        }
        return true;
    }

    public function transformWithEmbargo($xacml_xml)
    {
        if ($this->isPolicyEmbargoed()) {
            $xacml_dom = islandora_scholar_embargo_dom_document_load($xacml_xml);
            $xacml_xsl = XacmlCreateXslt::getInstance();
            $xsl_proc = $xacml_xsl->getXsltProcessor();
            $xsl_proc->setParameter('', 'users', implode(',', $this->reapply_embargo_users));
            $xsl_proc->setParameter('', 'dsids', implode(',', $this->reapply_embargo_dsids));
            $xacml_string = $xsl_proc->transformToXml($xacml_dom);
            return $xacml_string;
        } else {
            throw new Exception("Original Policy of $this->pid not embargoed");
        }
    }
}
