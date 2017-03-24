<?php

/**
 * Implements hook_rules_action_info().
 */
function trace_ext_workflow_rules_action_info() {
    $rules = array();

    $rules['trace_ext_workflow_rules_object_policy_add'] = array(
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
    $rules['trace_ext_workflow_rules_suppl_ds_update'] = array(
        'label' => t('Keep original DS label if SUPPL DS updated'),
        'group' => t('TRACE'),
        'parameter' => array(
            'object' => array(
                'type' => 'islandora_datastream',
                'label' => t('Updated datastream'),
                'description' => t('The label of a DS is immutable after ingestion.'),
            ),
        ),
        'provides' => array(
            'datastream' => array(
                'type' => 'islandora_datastream',
                'label' => t('Modified datastream instance'),
            ),
        ),
    );
    return $rules;
}
