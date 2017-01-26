<?php

/**
 * Implements hook_rules_action_info().
 */
function trace_workflow_policy_rules_action_info() {
    $rules = array();

    $rules['trace_workflow_policy_rules_object_policy_add'] = array(
        'label' => t('Add a Policy datastream on an object.'),
        'group' => t('TRACE'),
        'parameter' => array(
            'object' => array(
                'type' => 'islandora_object',
                'label' => t('Object'),
                'description' => t('A Tuque object for the Fedora object on which to add the datastream, as an entity.'),
            ),
        ),
        'provides' => array(
            'datastream' => array(
                'type' => 'islandora_datastream',
                'label' => t('Loaded datastream instance'),
            ),
        ),
    );
    return $rules;
}
