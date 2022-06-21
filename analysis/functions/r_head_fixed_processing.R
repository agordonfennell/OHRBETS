# Title     : r_rotary.R
# Objective : to house functions used for extraction / pre processing rotary files
# Created by: Adam Gordon-Fennell (agg2248@uw.edu)
#             Garret Stuber Lab, University of Washington
# Created on: 04/16/2021

# required functions
require(tidyverse)
require(lubridate)
require(readxl)
require(feather)

# determine file date
file_date <- function(x){
  x <- x %>%
    mutate(temp_file_name = file_name) %>%
    separate(temp_file_name, into = c('date_year', 'date_month', 'date_day', 'subject'), sep = "_") %>%
    mutate(date = str_c(date_year, date_month, date_day, sep = "_")) %>%
    select(-date_year, -date_month, -date_day) %>%
    select(file_name, subject, date, everything())
  
  return(x)
}

# extraction for each of the data types within the serial output --------------------------
#  note: if additional data types are added, add functions here and call them in extract_serial_output
# extract unidimensional parameters
extract_param <- function(x){
  param <- x %>%
    filter(id_type == 'param') %>%
    rename(value = event_ts) %>%
    select(-event_id) %>%
    spread(event_id_char, value)
  
  param <- param %>% file_date()
  
  if(dim(param)[1] > 0){
  
    return(param)} else {
    return(NA)
  }
}

# extract parameters that can be dynamic throughout the session (reinforcement direction, availability, etc.)
#  note: must use 2 line notation (code, ts; code value)
extract_param_dynamic <- function(x){
  param_dynamic <- x %>%
    filter(id_type == 'param_dynamic')
  
  if(dim(param_dynamic)[1] > 0){

  param_dynamic <- param_dynamic %>%
    mutate(param_id = rep(1:2, dim(param_dynamic)[1]/2)) %>%
    mutate(param_id = ifelse(param_id == 1, 'param_ts', 'param_value')) %>%
    mutate(param_num = rep(1:(dim(param_dynamic)[1]/2), each = 2)) %>%
    select(-event_id, -id_type) %>%
    spread(param_id, event_ts) %>%
    select(-param_num) %>%
    rename(param_dynamic = event_id_char)
  
  param_dynamic <- param_dynamic %>% file_date()

    return(param_dynamic)} else {
    return(NA)
  }
}

# extract events
extract_event <- function(x){
  event <- x %>%
    filter(id_type == 'event') %>%
    select(file_name, event_key, event_id, event_id_char, event_ts)
  
  event <- event %>% file_date()
  
  if(dim(event)[1]>0){
    return(event)} else {
    return(NA)
    }
}

# convert raw data into extracted data sets defined above
extract_serial_output <- function(dir_raw, key_events, dir_processed, manual_experiments, manual_blocknames, file_format_output = 'csv', overwrite){
  # dir_raw: input folder location that contains all raw data for the experiment
  # key_events: arduino event key tibble
  # dir_processed: output folder location to save individual files to
  #   - should be different than dir_raw
  # exp: unique expeirment name string that can identify relevant file names from within dir_raw
  #   - example: 2021_04_16_exp01_mouse02, use exp = "exp01" to extract all files that include that string
  #   - if you want to extract from multiple experiments, either change the file names (worse option),
  #     or extract separately into different dir_processed and then combine with an additional step (better option)
  # file_format_output: either csv or feather


  # create list of all files within dir_list
  dir_list <- list.files(dir_raw, pattern="*.csv")

  if(length(dir_list) == 0){
    print("WARNING: No files in dir_raw")
    return()
  }

  # filter if using manual_blocknames
  if(sum(!is.na(manual_blocknames)) > 0) {
    dir_list <- dir_list[dir_list %>% str_detect(manual_blocknames)]

    if(length(dir_list) == 0){
      print("No data in dir_raw matching manual_blocknames")
      print("")

      return()
    }
  }

  # filter list to only file names that contain exp
  if(sum(!is.na(manual_experiments)) > 0){
    dir_list <- dir_list[str_detect(dir_list, manual_experiments)]
  }
  # determine files already in dir_processed and remove from dir_list
  dir_processed_fns <- list.files(dir_processed)


  if(!overwrite){
    for(fn in dir_list){

      if(sum(str_detect(dir_processed_fns, fn %>% str_remove('.csv')) > 0)){
        dir_list <- dir_list[!str_detect(dir_list, fn)]
      }
    }
  }

  if(length(dir_list) > 0){
    if(overwrite){
      print("append override...")
      print(str_c('extracting following files found in dir ', dir_raw))
    } else {
      print(str_c('extracting following files not found in ', dir_processed))
    }

    for(dir in dir_list){
    print(dir)
    }

    print("")
  } else {
    print('no new serial output data to extract')
  }

  for(dir in dir_list){

    file_name <- basename(dir) %>% str_split("[.]")
    file_name <- file_name[[1]][1]

    print("--------------------------")
    print(str_c('extracting file: ', file_name))

    loop_data <- suppressWarnings( # supress parsing warning
      read_csv(str_c(dir_raw, '/', dir), col_names = c('event_id', 'event_ts'), col_types = cols()) %>%
        mutate(file_name = file_name)
    )

    if(sum(!is.na(loop_data %>% pull(event_ts))) == 0){
      loop_data <- loop_data %>%
        select(-event_ts) %>%
        separate(event_id, sep =" ", into = c('event_id', 'event_ts')) %>%
        mutate(event_id = as.numeric(event_id),
               event_ts = as.numeric(event_ts))
    }

  loop_data <- loop_data %>% select(file_name, event_id, event_ts) %>%
    left_join(key_events, by = 'event_id')


  loop_data_event         <- loop_data %>% extract_event()
  loop_data_param         <- loop_data %>% extract_param()
  loop_data_param_dynamic <- loop_data %>% extract_param_dynamic()

  print("")
  print('exported files: ')
  
  if(sum(!is.na(loop_data_event))>0){
    if(file_format_output == 'csv'){
      loop_data_event         %>% write_csv(str_c(dir_processed, file_name, '_event.csv'))
      print(str_c(dir_processed, file_name, '_event.csv'))
    }

    if(file_format_output == 'feather'){
      loop_data_event         %>% write_feather(str_c(dir_processed, file_name, '_event.feather'))
      print(str_c(dir_processed, file_name, '_event.feather'))
    }

  } else {
    print(str_c(file_name, ' has no events in serial data'))
  }
  
  if(sum(!is.na(loop_data_param))>0){
    if(file_format_output == 'csv'){
      loop_data_param         %>% write_csv(str_c(dir_processed, file_name, '_param.csv'))
      print(str_c(dir_processed, file_name, '_param.csv'))
    }

    if(file_format_output == 'feather'){
      loop_data_param         %>% write_feather(str_c(dir_processed, file_name, '_param.feather'))
      print(str_c(dir_processed, file_name, '_param.feather'))
    }

  } else {
    print(str_c(file_name, ' has no params in serial data'))
  }
  
  if(sum(!is.na(loop_data_param_dynamic))>0){
    if(file_format_output == 'csv'){
      loop_data_param_dynamic %>% write_csv(str_c(dir_processed, file_name, '_param_dynamic.csv'))
      print(str_c(dir_processed, file_name, '_param_dynamic.csv'))
    }

    if(file_format_output == 'feather'){
      loop_data_param_dynamic %>% write_feather(str_c(dir_processed, file_name, '_param_dynamic.feather'))
      print(str_c(dir_processed, file_name, '_param_dynamic.feather'))
    }

  } else {
    print(str_c(file_name, ' has no dynamic params in serial data'))
  }
  }
}

# processing -----------------------------------------------------------------------------------------------------------


process_multi_spout <- function(dir_extraction, dir_processed, log_data, log_multi_spout_ids, file_format_output, manual_fns = NA, overwrite = 0, time_bin_width = 25, time_bin_range = c(0,3000)){
  # process each individual file in dir_extracted and save in dir_processed
  #
  # inputs:
  #  - dir_extraction (string): path to extracted datasets for each session ending with forward slash '/' (e.g. ./data/extracted/)
  #  - dir_processed (string): output directory for processed datasets
  #  - log_data (dataframe): variables for the file_name, experiment, cohort, date used for assigning spout ids listed in log_multi_spout_ids
  #  - log_multi_spout_ids (df): variables for date, experiment, cohort, date, spout, and solution
  #  - file_format_output output file format ('csv' or 'feather')
  #  - manual_fns (vector of strings): vector of file names to process, use NA to process all data in dir_extraction
  #     ***note: use manual_fns or store data of different types in unique folders to prevent processing data with incorrect preprocessing pipeline
  #  - overwrite (logical): 0: only process data not yet processed, 1: overwrite existing datasets
  #  - time_bin_width (double): time in ms for the length of time bins used for data_trial_binned
  #  - time_bin_range (2 element vector, double): time window relative to trial onset for computing binned counts for data_trial_binned

  print("batch processing multi spout data...")

  # determine unique sessions  in dir_extracted
  dir_extraction_fns <- list.files(dir_extraction)

  dir_extraction_fns <- dir_extraction_fns[str_detect(dir_extraction_fns, '_event') & !str_detect(dir_extraction_fns, 'combined')] %>%
    str_remove(str_c('_event.', file_format_output) )


  # use manual_fns to filter to predetermined set
  if(sum(!is.na(manual_fns)) > 0){
    dir_extraction_fns <- dir_extraction_fns[dir_extraction_fns %in% manual_fns]
  }

  # determine files already in dir_processed and remove from dir_extraction_fns
  dir_processed_fns <- list.files(dir_processed)

  if(overwrite != 1){
    for(dir_extraction_fn in dir_extraction_fns){
      if(sum(str_detect(dir_processed_fns, dir_extraction_fn) > 0)){
        dir_extraction_fns <- dir_extraction_fns[!str_detect(dir_extraction_fns, dir_extraction_fn)]
      }
    }
  }

  if(length(dir_extraction_fns) == 0){
    print(str_c("all files in dir ", dir_extraction, ' are already processed and saved in dir ', dir_processed))
    return()
  }

  for(fn in dir_extraction_fns){
    print(str_c("processing fn: ", fn))

    # read in combined data
    data          <- read_feather(str_c(dir_extraction, fn, '_event.feather'))
    param_dynamic <- read_feather(str_c(dir_extraction, fn, '_param_dynamic.feather'))

    # set parameters
    trial_start_id     <- c('spout_extended')
    events_of_interest <- c('lick', 'lick_02', 'lick_03', 'lick_04', 'lick_05') # these events will be included in data_trial

    # generate trial ids
    trial_ids          <- generate_trial_ids_multispout(data, param_dynamic, trial_start_id)

    # generate trial summaries
    data_trial         <- generate_trial_events(data, trial_ids, trial_start_id, events_of_interest) # events relative to trial onsets
    data_trial_summary <- generate_trial_summary_multispout(data_trial, trial_ids) # trial summary
    data_spout_summary <- generate_session_spout_summary_multispout(data_trial_summary)
    data_trial_binned  <- generate_trial_binned_counts(data_trial, trial_ids, time_bin_width, time_bin_range) # binned lick counts

    # join multi-spout solution ids and create solution values
    data_trial           <- data_trial           %>% join_multi_spout_solution_id(log_data, log_multi_spout_ids)
    data_trial_summary   <- data_trial_summary   %>% join_multi_spout_solution_id(log_data, log_multi_spout_ids)
    data_spout_summary   <- data_spout_summary   %>% join_multi_spout_solution_id(log_data, log_multi_spout_ids)
    data_trial_binned    <- data_trial_binned    %>% join_multi_spout_solution_id(log_data, log_multi_spout_ids)

    data_trial           <- data_trial           %>% create_solution_value()
    data_trial_summary   <- data_trial_summary   %>% create_solution_value()
    data_spout_summary   <- data_spout_summary   %>% create_solution_value()
    data_trial_binned    <- data_trial_binned    %>% create_solution_value()

    print(str_c('  - saving files to dir: ', dir_processed))

    print(str_c('    ~ ', fn, '_data_trial.', file_format_output))
    print(str_c('    ~ ', fn, '_trial_summary.', file_format_output))
    print(str_c('    ~ ', fn, '_spout_summary.', file_format_output))
    print(str_c('    ~ ', fn, '_data_trial_binned.', file_format_output))

    if(file_format_output == 'feather'){
      data_trial           %>% write_feather(str_c(dir_processed, fn, '_data_trial.', file_format_output))
      data_trial_summary   %>% write_feather(str_c(dir_processed, fn,  '_trial_summary.', file_format_output))
      data_spout_summary   %>% write_feather(str_c(dir_processed, fn,  '_spout_summary.', file_format_output))
      data_trial_binned    %>% write_feather(str_c(dir_processed, fn, '_data_trial_binned.', file_format_output))
    }
    if(file_format_output == 'csv'){
      data_trial           %>% write_csv(str_c(dir_processed, fn, '_data_trial.', file_format_output))
      data_trial_summary   %>% write_csv(str_c(dir_processed, fn,  '_trial_summary.', file_format_output))
      data_spout_summary   %>% write_csv(str_c(dir_processed, fn,  '_spout_summary.', file_format_output))
      data_trial_binned    %>% write_csv(str_c(dir_processed, fn, '_data_trial_binned.', file_format_output))
    }

  }
}

# Processing execution -------------------------------------------------------------------------------------------------
process_arduino <- function(data, key_events, log_data, dir_processed, wheel_diameter = 63){
  # data: output    from combine_extracted_serial_output()
  # key_events:     arduino event key tibble
  # log_data:       log for each file within the data
    # must contain following
    #  - ppr (pulse per rotation of rotary encoder)
    #  - resolution (number of pulses per recorded rotation)
  # dir_processed:  folder location that extract_serial_output() exported extracted files to
  # wheel_diameter: diameter of operant wheel (63 for standard design)


  
  # join event key to data
  data <- data %>%
    left_join(key_events) %>%
    select(-id_type)

  # compute distances for rotation and ratios using data log
  log_data <- log_data %>%
    mutate(wheel_circumfrence = pi * wheel_diameter) %>%
    mutate(resolution_rotation = 1 / (ppr / resolution)) %>%
    mutate(rotation = resolution_rotation * wheel_circumfrence) %>%
    rename(file_name = beh_fn)

  # join data log to data
  data <- data %>%
    mutate(date = ymd(date)) %>%
    left_join(log_data)

  # create cumulative event count
  data_rotation <- data %>%
    filter(event_id_char %in% c('active_rotation', 'inactive_rotation')) %>%
    group_by(file_name, event_id, event_id_char) %>%
    arrange(file_name, event_ts) %>%
    mutate(count_cummulative = row_number()) %>%
    mutate(rotation_relative      = cumsum(rotation),
           rotation_relative_turn = cumsum(resolution_rotation)) %>%
    mutate(rotation_absolute      = ifelse(event_id_char == 'inactive_rotation', -1 * rotation,            1 * rotation),
           rotation_absolute_turn = ifelse(event_id_char == 'inactive_rotation', -1 * resolution_rotation, 1 * resolution_rotation)) %>%
    group_by(file_name) %>%
    mutate(rotation_absolute      = cumsum(rotation_absolute),
           rotation_absolute_turn = cumsum(rotation_absolute_turn)
           ) %>%
    ungroup()

  # create summary
  data_summary <- data %>%
    group_by(file_name, event_id_char, rotation, resolution_rotation) %>%
    summarise(event_count = n()) %>%
    mutate(rotation      = ifelse(event_id_char %in% c('active_rotation', 'inactive_rotation'), event_count * rotation, NA)) %>%
    mutate(rotation_turn = ifelse(event_id_char %in% c('active_rotation', 'inactive_rotation'), event_count * resolution_rotation,NA)) %>%
    select(-resolution_rotation) %>%
    ungroup()

  # save outputs
  data          %>% write_feather(str_c(dir_processed, 'combined_event_processed.feather'))
  data_rotation %>% write_feather(str_c(dir_processed, 'combined_rotation.feather'))
  data_summary  %>% write_feather(str_c(dir_processed, 'combined_summary.feather'))
}



# Multi-spout functions ------------------------------------------------------------------------------------------------
generate_trial_ids_multispout <- function(data, param_dynamic, trial_start_id){
  # function uses data and param_dynamic to extract spout for each access period
  # trial_start_id defines the event_id_char value that denotes the start of a trial

  # returns tibble with
  #    file_name:      filename string (key value)
  #    trial_start_ts: time of trial start
  #    trial_num:      number of trial
  #    trial_id:       identification of trial (spout id)

  # extract trial_id (spout / trial) from param_dynamic
  join_current_pos <- param_dynamic %>%
    select(file_name, param_dynamic, param_ts, param_value) %>%
    filter(param_dynamic == 'current_pos') %>%                     # filter to current position of radial spout
    filter(!is.na(param_value)) %>%                                # remove
    rename(trial_id = param_value) %>%
    mutate(trial_id = str_c('spout0', as.character(trial_id + 1))) %>%  # convert spout_id value to spout_id character
    arrange(file_name, param_ts) %>%     # arrange by param_ts to determine trial number
    group_by(file_name) %>%
    mutate(trial_num = row_number()) %>% # generate trial_num based on row
    select(-param_ts, -param_dynamic)

  # gernate trial ids from data and join_current_pos
  trial_ids <- data %>%
    select(file_name, event_id_char, event_ts) %>%
    filter(event_id_char %in% trial_start_id) %>% # filter to spout_extended for trial onsetd
    group_by(file_name) %>%                # for each file_name
    filter(event_ts > min(event_ts)) %>%   # remove first event (this will remove spout extention setup)
    arrange(file_name, event_ts) %>%       # arrange by event_ts to determine trial number
    mutate(trial_num = row_number()) %>%   # generate trial_num based on row
    left_join(join_current_pos, by = c("file_name", "trial_num")) %>%
    rename(trial_start_ts = event_ts) %>%
    select(-event_id_char) %>%
    group_by(file_name, trial_id) %>%
    mutate(trial_num_tastant = row_number()) %>%
    ungroup()

  return(trial_ids)
}

generate_trial_summary_multispout <- function(data_trial, trial_ids){
  # uses data_trial and  trial_ids to compute summary statitics for each trial
  #
  # returns tibble with
  #   file_name: file name string (key value)
  #   trial_start_ts: start time for each trial (obtained from trials_ids)
  #   trial_num:      number of trial (obtained from trials_ids)
  #   trial_id:       id of trial (obtained from trials_ids)
  #   lick_count:     number of licks during a trial
  #   lick_ts_first   relative timestamp of first lick (ms)
  #   lick_ts_last    relative timestamp of last lick (ms)
  #   lick_ili_mean   mean interlick interval for trial

  # compute trial data summary from data defined in data_trial
  data_trial_summary <- data_trial %>%
    group_by(file_name, trial_start_ts, trial_num, trial_num_tastant, trial_id) %>%
    arrange(file_name, trial_start_ts, event_ts) %>%
    mutate(lick_ili = event_ts - lag(event_ts)) %>%
    summarise(lick_count = n(),
              lick_ts_first = min(event_ts_rel),
              lick_ts_last  = max(event_ts_rel),
              lick_ili_mean = mean(lick_ili, na.rm = TRUE),
              .groups = "drop" ) %>%
    mutate(trial_lick = 1)

  # join summary to trial_ids in order to avoid implicit 0s
  data_trial_summary <- data_trial_summary %>%
    left_join(trial_ids, ., by = c("file_name", "trial_start_ts", "trial_num", 'trial_num_tastant', "trial_id")) %>%
    mutate(lick_count = ifelse(is.na(lick_count), 0, lick_count), # fill in 0s for trials without a lick listed in data_trial
           trial_lick = ifelse(is.na(trial_lick), 0, trial_lick)) # ...

  return(data_trial_summary)
}


generate_session_spout_summary_multispout <- function(data_trial_summary){
  data_session_summary <- data_trial_summary %>%
    group_by(file_name, trial_id) %>%
    summarise(lick_count_trial_mean = lick_count %>% mean(),
              lick_count_total      = lick_count %>% sum(),
              lick_ts_first_mean    = lick_ts_first %>% mean(na.rm = TRUE),
              lick_ts_last_mean     = lick_ts_last %>% mean(na.rm = TRUE),
              lick_ili_mean_mean    = lick_ili_mean %>% mean(na.rm = TRUE),
              trial_lick_proportion = sum(trial_lick) / n(),
              .groups = "drop" 
              )

  return(data_session_summary)
}

join_multi_spout_solution_id <- function(df, log_data, log_multi_spout_ids){

  if(sum(str_detect(colnames(df), 'trial_id')) == 1){
    df <- df %>% rename(spout = trial_id)
  }

  df %>%
    left_join(log_data %>% select(file_name = beh_fn, experiment, cohort, date),
              by = "file_name") %>%
    left_join(log_multi_spout_ids,
              by = c("experiment", "cohort", "date", "spout"))

}

create_solution_value <- function(df){
  df<-
    suppressWarnings( # supress warning of as.double(NA)
      df %>%
      mutate(solution_type = ifelse(str_detect(solution, 'sucrose'),
                                    'sucrose',
                                     NA)
             ) %>%
      mutate(solution_type = ifelse(str_detect(solution, 'nacl'),
                                     'nacl',
                                     solution_type)
             ) %>%
      mutate(solution_type = ifelse(str_detect(solution, 'quinine'),
                                     'quinine',
                                     solution_type)
             ) %>%
      mutate(solution_value = ifelse(str_detect(solution, 'sucrose'),
                                     str_remove(solution, 'sucrose') %>% as.double(),
                                     NA)
             ) %>%
      mutate(solution_value = ifelse(str_detect(solution, 'nacl'),
                                     str_remove(solution, 'nacl') %>% as.double() / 100,
                                     solution_value)
             ) %>%
      mutate(solution_value = ifelse(str_detect(solution, 'quinine'),
                                     str_remove(solution, 'quinine') %>% as.double() / 1000,
                                     solution_value)
             )
    )


  return(df)
}

# Trial functions -----------------------------------------------------------------------------------------------------
# these functions might belong in a more general R function repository
generate_trial_events <- function(data, trial_ids, trial_start_id, events_of_interest){
  # uses data and trial_ids to determine the trial info for each event defined by events_of_interest
  # * trial_start_id defines the event_id_char value that denotes the start of a trial, must match the one
  #   used to generate the trial_ids
  #
  # returns tibble with
  #   file_name: file name string (key value)
  #   event_id_char: event names defined in key_events, filetered to events defined in events_of_interest
  #   event_ts: time stamp in milliseconds for each event
  #   trial_start_ts: start time for each trial (obtained from trials_ids)
  #   trial_num:      number of trial (obtained from trials_ids)
  #   trial_id:       id of trial (obtained from trials_ids)
  #   event_ts_rel:   event_ts relative to trial_start_ts

  data_trial <- data %>%
    filter(event_id_char %in% c(events_of_interest, trial_start_id)) %>%
    select(file_name, event_id_char, event_ts) %>%
    left_join(trial_ids %>% mutate(event_ts = trial_start_ts), by = c("file_name", "event_ts")) %>%
    arrange(file_name, event_ts) %>%
    fill(c(trial_num, trial_num_tastant, trial_id, trial_start_ts)) %>%
    filter(!is.na(trial_num)) %>%
    filter(event_id_char %in% events_of_interest) %>%
    mutate(event_ts_rel = event_ts - trial_start_ts)


  return(data_trial)
}

generate_trial_binned_counts <- function(data_trial, trial_ids, time_bin_width, time_bin_range){
  # returns binned counts for observations in data_trial
  # empty trials are completed using trial_num within trial_ids
  #  - data_trial produced by generate_trial_events()
  #  - trial_ids produced by genrate_trial_ids_*()
  #  - time_bin_width: width of time bin (ms)
  #  - time_bin_range: 2 element vector with start and end of range (ms)

  grouping_vars  <- c('file_name', 'trial_id')
  var_trial      <- 'trial_num'
  var_time_stamp <- 'event_ts_rel'

  # find files to analyze that are in both trial_ds and data_trial
  files_to_process <- inner_join(
    trial_ids   %>% select(file_name) %>% unique(),
    data_trial  %>% select(file_name) %>% unique(),
    by = 'file_name') %>%
    pull(file_name)

  # for each file, compute binned counts
  for (file_name_loop in files_to_process){
    trial_ids_loop <- trial_ids %>%
      filter(file_name == file_name_loop)

    data_trial_loop <- data_trial %>%
      filter(file_name == file_name_loop)

    n_trials <- nrow(trial_ids_loop)

    data_trial_binned_loop <- data_trial_loop %>%
      mutate(time_bin = cut(event_ts_rel,
                          seq(time_bin_range[1], time_bin_range[2], time_bin_width),
                          labels = seq(time_bin_range[1], time_bin_range[2]-time_bin_width, time_bin_width))) %>%
      mutate(time_bin = time_bin %>% as.character() %>% as.double()) %>%
      filter(!is.na(time_bin)) %>%
      group_by(file_name, trial_start_ts, trial_num, trial_id, time_bin) %>%
      summarise(count_binned = n(), .groups = 'drop') %>%
      group_by(file_name) %>%
      select(file_name, trial_num, time_bin, count_binned) %>%
      complete(trial_num = 1:n_trials,
               time_bin = seq(time_bin_range[1], time_bin_range[2]-time_bin_width, time_bin_width),
               fill = list(count_binned = 0)) %>%
      left_join(trial_ids_loop, by = c('file_name', 'trial_num'))

    if(file_name_loop == files_to_process[1]){
      data_trial_binned <- data_trial_binned_loop
    } else {
      data_trial_binned <- data_trial_binned_loop %>% bind_rows(data_trial_binned,.)
    }
  }
  return(data_trial_binned)
}


