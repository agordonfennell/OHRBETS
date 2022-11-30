#%%
import numpy as np
import pandas as pd
import random
from time import localtime, strftime

# parameters
trial_types =  [0,1,2,3,4]   # integers for different types of trials
trial_counts = [2,2,2,2,2]  # number of each trial type
session_splits = 10

n_sessions = 1

save_csv = False


session_vector_output = []
session_df_output = pd.DataFrame()

for n_session in np.arange(n_sessions):

    # program
    trial_vector_output = []


    for session_split in np.arange(session_splits):
        trial_vector = [] # generate vector
        
        for n_trial_type in np.arange(0,len(trial_types)): # for each trial type
            trial_vector = np.append(trial_vector, np.zeros(trial_counts[n_trial_type]) + trial_types[n_trial_type]) # create a vector with value = trial type and length = trial count

        repeat_criteria = 1

        random.shuffle(trial_vector)  # shuffle split vector

        trial_vector_output = np.append(trial_vector_output,trial_vector)

    trial_vector_output = trial_vector_output.astype(int)
    output_string = ','.join(str(x) for x in trial_vector_output) 
    output_string = "{" + output_string + "};"

    session_vector_output.append(output_string)

    session_df_output = session_df_output.append(pd.DataFrame({'n_session': np.repeat(n_session, len(trial_vector_output)) , 'trial_id':trial_vector_output}), ignore_index=True)

    print("session " + str(n_session + 1))
    print()
    print(output_string)
    print()
    print('---------------')


if save_csv:
    session_vector_output = pd.DataFrame(session_vector_output, columns = ['trial_ids'])
    session_vector_output.to_csv(strftime("%Y_%m_%d", localtime()) + '_session_ids_string.csv',
        index = False)
    session_df_output.to_csv(strftime("%Y_%m_%d", localtime()) + '_session_ids_df.csv',
        index = False)


