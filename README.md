# Trace Ext Workflow
This module extends the workflow for the community manager for managing user submitted electronic thesis and dissertations(ETD).

## What does this module do?
When a student submits a ETD a entry to the trace_workflow_pids table is created and adds a policy datastream. When the user submits a replacement datastream, edits the submission form, or adds a new datastream the table is update and sorted by latest edited.

When the manager __'accepts'__ the the submission the status is changed and removed from the manager's list and is now pending to be published.

![screenshot-localhost-8000-2017-12-11-16-25-17-754](https://user-images.githubusercontent.com/2738244/33854498-ec50c5a0-de8f-11e7-8c9a-69f94c8a6690.png)

### KEY
+ __'pid'__ : of the object
+ __'label'__ : is the title the user gave during submission
+ __'state'__ : has 2 states __'s'__ for submitted and __'a'__ for active
+ __'username'__ : the username of the person submitting
+ __'modified_date'__ : this is automatically updated to the server's timestamp when any of the row is updated.
+ __'datastream'__ : this is a timestamp of when a datastream has been uploaded
+ __'representative_body'__ :  __T__ (Thesis), __D__ (Dissertation) or __NULL__ (Unknown).

## /trace_ext_workflow/list
Manager will require access rights to view this list.

![screenshot-localhost-8000-2017-12-11-16-20-30-662](https://user-images.githubusercontent.com/2738244/33854310-416a325c-de8f-11e7-87ec-a0d419343593.png)

## POLICY datastream
The are two rules that need to be configured __trace_ext_workflow_rules_object_policy_add__ and  __trace_ext_workflow_rules_suppl_ds_update__.

+ Go to admin/config/workflow/rules and "Add new rule"
+ Give it a name and select 'Object ingested' from the React on event dropdown menu.
+ Add 'Add action'
+ Select TRACE > 'Add a Policy datastream on an object.'

![screenshot-localhost-8000-2017-12-11-16-43-14-967](https://user-images.githubusercontent.com/2738244/33855269-6ce047c0-de92-11e7-8a80-2e287034dc32.png)

#### Use the following settings.
* Data selector: object
* Variable label: Loaded datastream instance
* Variable name: datastream

## Publish with simple_workflow
![screenshot-localhost-8000-2017-12-11-16-50-19-516](https://user-images.githubusercontent.com/2738244/33855603-660606a0-de93-11e7-8f22-a05b74479c22.png)
