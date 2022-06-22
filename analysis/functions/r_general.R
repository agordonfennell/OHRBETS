# Title     : f_general_functions.R
# Objective : collect functions for functions that are used across multiple applications
# Created by: Adam Gordon-Fennell (agg2248@uw.edu)
#             Garret Stuber Lab, University of Washington
# Created on: 1/31/2022

# general --------------------------------------------------------------------------------------------------------------
# dir_diff (used in tidy_med, fp_preprocessing)
dir_diff <- function(dir_input, dir_output, list_suffix, print_message = 0, ignore_suffixes = NA){
  # return the files located in dir_input that are not already located in dir_output
  #
  # list_suffix provides strings that contain file name suffix + extension to be removed off of file name

  list_dir_input <- list.files(dir_input)
  list_dir_output <- list.files(dir_output)

  # filter out files in dir_output that contain suffix to ignore
  if(sum(!is.na(ignore_suffixes)) >= 1){
    for(ignore_suffix in ignore_suffixes){
      list_dir_output <- list_dir_output[!str_detect(list_dir_output, ignore_suffix)]
    }
  }

  for(suffix in list_suffix){
    list_dir_input <- list_dir_input %>%
      str_remove(suffix)

    list_dir_output <- list_dir_output %>%
      str_remove(suffix)
  }

  list_dir_input <- list_dir_input %>% unique()
  list_dir_output <- list_dir_output %>% unique()

  # if there are any files in the output directory that are not in the input directory, give a warning
  if(length(setdiff(list_dir_output, list_dir_input))){
    if(print_message){
      print("WARNING: THE FOLLOWING FILES ARE LOCATED IN THE dir_output THAT ARE NOT FOUND IN dir_input")
    }

    for(fn_missing in setdiff(list_dir_output, list_dir_input)){
      if(print_message){
        print(fn_missing)
      }
    }
    print("")
  }

  # create a list of files contained in dir_input but not dir_output
  fns <- setdiff(list_dir_input, list_dir_output)

  if(print_message){
    print(str_c("number of files not found in dir_output: ", length(fns)))
    print("")
  }

  return(fns)
}

# generic combine / append function
combine_files <- function(dir_input_tidy, fns, output_fn, suffix, append_combined){
  # combines multiple files into a single combined file. If the combined file already exists, new files will be appended
  # to the existing combined file
  #
  # dir_input_tidy: directory of input tidy data
  # fns: list of files within dir_input_tidy that we will combine
  # suffix: suffix of input files and output file (e.g. "_events.csv")
  # append_combined: 1: append to existing combined dataframe, 0: genreate from scratch and overwrite existing dataframe

  skip <- 0
   if(append_combined){
      if(file.exists(output_fn)){
        if(str_detect(suffix, 'csv')){fns_combined <- read_csv(output_fn, col_types = cols())}
        if(str_detect(suffix, 'feather')){fns_combined <- read_feather(output_fn)}

        fns_combined <- fns_combined %>% pull(med_file_name) %>% unique()

        fns_diff <- setdiff(fns %>% str_remove(suffix), fns_combined)

        if(length(fns_diff) > 0){
          fns_diff <- fns_diff %>%
            str_c(suffix)
        }

        if(length(fns_diff) == 0){
          print(str_c("all raw files already found in combined dataframe ", output_fn))
          skip <- 1
        } else {
          fns <- fns_diff
        }
      } else {
        fns_diff <- vector('character')
      }
    } else {
      print("append overwrite... creating new output dataframe")
      fns_diff <- vector('character')
    }

    if(length(fns) == 0){
      print(str_c("WARNING NO FILES FOUND WITH fn_string = ", fn_string, " & suffix = ", suffix))
      next
    }

  if(skip != 1){
      print(str_c("combining ", length(fns), " files with suffix ", suffix))

      first_file <- 1

      for(fn in fns){
        if(str_detect(fn, 'csv')){df_loop <- read_csv(str_c(dir_input_tidy, '/', fn), col_types = cols())}
        if(str_detect(fn, 'feather')){df_loop <- read_feather(str_c(dir_input_tidy, '/', fn))}
        if(!str_detect(fn, 'csv') & !str_detect(fn, 'feather')){
          print(str_c("WARNING INCOMPATABLIE FILE TYPE FOR FILE ", fn))
          next
        }

        if(first_file == 1){
          df_out <- df_loop
          first_file <- 0
        } else {
          df_out <- df_loop %>% bind_rows(df_out, .)
        }
      }

      print(str_c("writing: ", output_fn))

      if(str_detect(suffix, 'csv')){
        if(append_combined & length(fns_diff) > 0){
          print(output_fn)
          print(read_csv(output_fn))
          print(df_out)
          df_out %>% bind_rows(read_csv(output_fn, col_types = cols()),.) %>% write_csv(output_fn)
        } else {
          df_out %>% write_csv(output_fn)
        }
      }

      if(str_detect(suffix, 'feather')){
        if(append_combined & length(fns_diff) > 0){
          df_out %>% bind_rows(read_feather(output_fn, col_types = cols()),.) %>% write_feather(output_fn)
        } else {
          df_out %>% write_feather(output_fn)
        }
      }

      print("")
    }
  }

coerce_character <- function(t1, t2){
  # compare the datatypes of 2 dataframes and then convert numeric data to character if there is a disagreement
  #

  t1_dtype <- t1 %>%
    head() %>%
    collect() %>%
    lapply(class) %>%
    unlist() %>%
    as_tibble() %>%
    filter(value != 'POSIXt') %>% # filter out double data type for dates
    rename(t1_dtype = value) %>%
    mutate(variable = colnames(t1))


  t2_dtype <- t2 %>%
    head() %>%
    collect() %>%
    lapply(class) %>%
    unlist() %>%
    as_tibble() %>%
    filter(value != 'POSIXt') %>% # filter out double data type for dates
    rename(t2_dtype = value) %>%
    mutate(variable = colnames(t2))

  diff_t1_t2 <-
    suppressMessages(left_join(t1_dtype, t2_dtype)) %>%
    filter(t1_dtype != t2_dtype)

  if(nrow(diff_t1_t2 > 0)){
    for(n_var in 1:nrow(diff_t1_t2)){
      var_name <- diff_t1_t2$variable[n_var]
      if(diff_t1_t2$t1_dtype[n_var] == 'character'){
        t2 <- t2 %>% mutate_at(vars(var_name), as.character)
      }

      if(diff_t1_t2$t2_dtype[n_var] == 'character'){
        t1 <- t1 %>% mutate_at(vars(var_name), as.character)
      }
    }
  }

  return(list(t1, t2))
}

combined_import <- function(import_directory, filter_strings = NA, prefixes = NA, suffixes){
  # combine a set of csv or feather files into a single dataframe
  #
  # inputs:
  # - import_directory: directory of files
  # - filter_strings (OPTIONAL): vector of strings to filter files in import_directory with
  # - prefix (OPTIONAL): vector of strings to filter files in import_directory with
  # - suffixes: vector of strings of suffixes you wish to combine
  #
  # output:
  # - list of dataframes that combine files with specified suffixes

  if(length(filter_strings) == 0){
    print('combined_import cancled: filter_strings has length of 0')
    return(0)
  }

  num_suffix <- 1

  for(suffix in suffixes){
    fns <- list.files(import_directory)

    # filter fns to files with  strings included in filter_strings
    if(sum(!is.na(filter_strings)) > 0){
      for(filter_string in filter_strings){
        fns_loop <- fns[str_detect(fns, filter_string)]

        if(filter_string == filter_strings[1]){
          fns_combined <- fns_loop
        } else{
          fns_combined <- c(fns_combined, fns_loop)
        }
      }
    fns <- fns_combined
    }

    # filter fns to files with  strings included in prefixes
    if(sum(!is.na(prefixes)) > 0){
      for(prefix in prefixes){
        fns_loop <- fns[str_detect(fns, prefix)]

        if(prefix == prefixes[1]){
          fns_combined <- fns_loop
        } else{
          fns_combined <- c(fns_combined, fns_loop)
        }
      }
    fns <- fns_combined
    }

    # filter fns to files with suffix
    fns <- fns[str_detect(fns, suffix)]

    # read in and combine files in fitler_list
    if(str_detect(suffix, 'feather')){
      for(fn in fns){
        data_loop <- read_feather(str_c(import_directory, '/', fn)) %>%
          mutate(fn = fn) %>%
          select(fn, everything())

        if(fn == fns[1]){
          data_combined <- data_loop
        } else {
          data_combined_list <- coerce_character(data_combined, data_loop)

          data_combined <- data_combined_list[[1]]
          data_loop     <- data_combined_list[[2]]

          data_combined <- data_loop %>% bind_rows(data_combined,.)
        }
      }
    }

    if(str_detect(suffix, 'csv')){
      for(fn in fns){
        data_loop <- read_csv(str_c(import_directory, '/', fn)) %>%
          mutate(fn = fn) %>%
          select(fn, everything())

        if(fn == fns[1]){
          data_combined <- data_loop
        } else {
          data_combined_list <- coerce_character(data_combined, data_loop)

          data_combined <- data_combined_list[[1]]
          data_loop     <- data_combined_list[[2]]

          data_combined <- data_loop %>% bind_rows(data_combined,.)
        }
      }
    }

    if(num_suffix == 1){
      output_list <- list(data_combined)
      } else {
        output_list[[num_suffix]] <- data_combined
        }

    num_suffix <- num_suffix + 1
  }

  return(output_list)
}


# combine based on string in file name
combine_dfs_fn_string <- function(dir_input_tidy, dir_output_combined, list_suffix, fn_string, append_combined){
  # combine multiple tidy datasets into a single file
  #
  # inputs:
  # - dir_input_tidy (string): system directory of tidy datasets you wish to combine
  # - dir_output_combined (string): system directory of combined tidy datasets will be saved to
  # - list_suffix (vector of strings): file name suffixes contained in dir_input_tidy you wish to combine
  #   must include file extensions .csv or .feather
  #   NOTE: the input suffix will be used to set the output data type (e.g. .csv in .csv out)
  # - fn_string (string): string contained in file names within dir_input_tidy you wish to combine into a single file
  # - append_combined (boolean): 1: append to existing combined dataframe, 0: genreate from scratch and overwrite
  #   existing dataframe
  #
  # outputs:
  #  - combined tidy datasets (1 / member of list_suffix)

  list_dir_input_tidy_files <- list.files(dir_input_tidy)

  list_dir_input_tidy_files <- list_dir_input_tidy_files[list_dir_input_tidy_files %>% str_detect(fn_string)]

  for(suffix in list_suffix){
    fns <- list_dir_input_tidy_files[list_dir_input_tidy_files %>% str_detect(suffix)]

    output_fn <- str_c(dir_output_combined, '/data_med_combined_', fn_string, suffix)

    combine_files(dir_input_tidy, fns, output_fn, suffix, append_combined)
  }
}

combine_dfs_log_variable <- function(dir_input_tidy, dir_output_combined, list_suffix, log_data, variables, values, append_combined){
  # combine multiple tidy datasets into a single file
  #
  # inputs:
  # - dir_input_tidy (string): system directory of tidy datasets you wish to combine
  # - dir_output_combined (string): system directory of combined tidy datasets will be saved to
  # - list_suffix (vector of strings): file name suffixes contained in dir_input_tidy you wish to combine
  #   must include file extensions .csv or .feather
  #   NOTE: the input suffix will be used to set the output data type (e.g. .csv in .csv out)
  # - log_data (dataframe): dataframe containing a variable with file names, and variables used to define data to combine
  # - variables (vector of strings): variable names contained in dataframe you wish to combine
  # - values (vector of strings): values of variables that you wish to filter to
  # - append_combined (boolean): 1: append to existing combined dataframe, 0: genreate from scratch and overwrite
  #   existing dataframe
  #
  # outputs:
  #  - combined tidy datasets (1 / member of list_suffix)

  # filter log_data that have variable(n) that matches value(n)
  for(n_filter in 1:length(variables)){
    log_data <- log_data %>%
      filter((!!sym(variables[n_filter])) == values[n_filter])
  }

  list_dir_input_tidy_files <- list.files(dir_tidy)

  for(suffix in list_suffix){
    list_dir_input_tidy_files <- list_dir_input_tidy_files %>%
      str_remove(suffix) %>%
      unique()

  }

  fn_stems <- setdiff(list_dir_input_tidy_files, log %>% pull(med_file_name))

  for(suffix in list_suffix){
    # generate file names
    fns <- fn_stems %>% str_c(suffix)

    # create output_fn
    output_fn <- str_c(dir_output_combined, '/data_med_combined')

    for(n_filter in 1:length(variables)){
      output_fn <- output_fn %>% str_c('__', variables[n_filter], '_', values[n_filter])
    }

    output_fn <- output_fn %>% str_c(suffix)

    combine_files(dir_input_tidy, fns, output_fn, suffix, append_combined)
  }
}

# Get events -----------------------------------------------------------------------------------------------------------
get_event_bouts <- function(events, event_id_char_of_interest,  filt_tm, filt_n, filt_dir, id_char){
  # extract either onset or offsets of bouts of a chosen event based on the number of events prior/post (filt_n) within
  # a timeframe prior/post (filt_dir)
  #
  # inputs:
  #  - events (df): dataset from a single session that includes event_id_char (the id of the event) and event_ts (the
  #    time of the event
  #  - event_id_char_of_ineterest (char vector): vector of strings that contains the events that you want to use
  #  - filt_tm (double vector): time for window prior and post event that will be used
  #      - Units of this variable must match units for event_ts in events
  #  - filt_n (integer vector): number of events in the window prior and post that will be used
  #  - filt_dir(character vector): logical used to apply to time window ('>', '<', or '==')
  #  - id_char (string): string to replace event_id_char in output dataframe
  #
  # directions:
  #  - filt_tm, filt_n, and filt_dir each contain 2 values that apply to the window prior to and post each event
  #  - events will be filtered down to events that match the specified conditions
  #
  # example:
  #  - licking bout onsets
  #  - condition: no licks in 1s prior to first lick, at least 1 lick in 1s post lick
  #  input: events_get_event_bouts(events, c('lick'), c(1,1), c(0, 0), c('==', '>'), 'lick_onset')

  # filter to data that match the event of interest
  events_bout_onset <- events %>%
    filter(event_id_char %in% event_id_char_of_interest) %>%
    mutate(n_event_prior = 0,
           n_event_post  = 0) %>%
    arrange(event_ts)

  event_tss <- events_bout_onset %>%
    pull(event_ts)

  # for each time stamp, retrieve the number of events preceding and following
  for (event in 1:nrow(events_bout_onset)){
    events_bout_onset$n_event_prior[event] <-
      sum(
        event_tss > events_bout_onset$event_ts[event] - filt_tm[1] &
        event_tss < events_bout_onset$event_ts[event]
        )

    events_bout_onset$n_event_post[event] <-
      sum(
        event_tss > events_bout_onset$event_ts[event] &
        event_tss < events_bout_onset$event_ts[event] + filt_tm[2]
        )
  }

  # filter based on events prior
  if(filt_dir[1] == '<'){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_prior < filt_n[1])
  }

  if(filt_dir[1] == '>'){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_prior > filt_n[1])
  }

  if(filt_dir[1] == '=='){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_prior == filt_n[1])
  }

  # filter based on events post
  if(filt_dir[2] == '<'){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_post < filt_n[2])
  }

  if(filt_dir[2] == '>'){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_post > filt_n[2])
  }

  if(filt_dir[2] == '=='){
    events_bout_onset <- events_bout_onset %>%
    filter(n_event_post == filt_n[2])
  }

  # rename event_id_char so it can be combined with input dataset
  events_bout_onset <- events_bout_onset %>%
    mutate(event_id_char = id_char) %>%
    select(-n_event_prior, -n_event_post)

  return(events_bout_onset)
}


# compute binned average of perievent data
return_event_binned_counts <- function(df_events, var_relative_ts, time_start, time_end, bin_width, ...){
  # return the binned counts for events within a dataframe
  #
  # inputs:
  # - df_events: dataframe that contains the relative time of events
  # - var_relative_ts: string of the variable name for relative time within df_peri_event
  # - time_start: time start (units match var_relative_ts in df_peri_event)
  # - time_end: time followig event that should be included in output dataframe (units match var_relative_ts in df_peri_event)
  # - bin_width: the width of time bins for computing binned counts (units match var_relative_ts in df_peri_event)
  # - ...: grouping variables
  #
  # output:
  # - peri_event_binned: dataframe with following varibles
  #   - grouping variables as defined by var_grouping
  #   - event_bin: left side of time bin
  #   - event_count: number of observations that occured during the corresponding bin
  var_relative_ts <- enquo(var_relative_ts)

  bins <- seq(time_start, time_end, bin_width)

  df_event_binned <- df_events %>%
    mutate(event_bin = cut(!!var_relative_ts, bins, label = bins[1:length(bins) - 1])) %>%
    mutate(event_bin = event_bin %>% as.character() %>% as.double()) %>%
    mutate(event_bin = event_bin %>% round(4) %>% as.character()) %>% # need to convert back to character to avoid slight numerical differences
    group_by(..., event_bin) %>%
    summarise(event_count = n()) %>%
    ungroup() %>%
    complete(..., event_bin = as.character(bins[1:length(bins) - 1]), fill = list(event_count = 0)) %>%
    mutate(event_bin = event_bin %>% as.double())

  return(df_event_binned)
}

# stats -----
## rm anova one within ------------------------------------------------------------------------------------------------
aov_rm_one_within <- function(df, dir_output, prefix, var_id, var_dependent, var_within){
  require(afex)

  anova_model <- aov_ez(
    id = var_id,
    dv = var_dependent,
    data = df,
    within = var_within,
    anova_table=list(correction="none", es = "none"))


  label_anova <- c("intercept", var_within) %>% as_tibble()

  anova_summary <-  summary(anova_model)[[4]][] %>%
                    as_tibble() %>%
                    bind_cols(label_anova,.) %>%
                    rename("effect" = "value",
                           "ss" = "Sum Sq",
                           "df_num" = "num Df",
                           "ss_error" = "Error SS",
                           "df_den" = "den Df",
                           "f" = "F value",
                           "p_value" = "Pr(>F)")

  anova_summary <- anova_summary %>%
    mutate(stat = 'aov_rm_one_within',
           var_dv      = var_dependent,
           var_id      = var_id,
           var_within  = var_within) %>%
    select(stat, var_dv, var_id, var_within, everything())


    interaction <- paste("~", var_within, sep = "")

    emm <- emmeans(anova_model, formula(interaction))

    pairs_hsd <- pairs(emm) %>%
      as_tibble() %>%
      separate(contrast, into = c('contrast1_within', 'contrast2_within'), sep = '-') %>%
      mutate_if(is.character, str_trim) %>%
      rename('p_value' = 'p.value',
             't_ratio' = 't.ratio')

  save_aov(dir_output, prefix, anova_summary, pairs_hsd, df)

  return(list(anova_model, anova_summary, pairs_hsd))
}

save_aov <- function(dir_output, prefix, anova_summary, pairs_hsd, df){
  if(!is.na(dir_output)){
    fn <- str_c(dir_output, '/', prefix, '_aov.csv')
    anova_summary %>% write_csv(fn)
    print(str_c('saved file: ', fn))

    fn <- str_c(dir_output, '/', prefix, '_aov_hsd.csv')
    pairs_hsd %>% write_csv(fn)
    print(str_c('saved file: ', fn))

    fn <- str_c(dir_output, '/', prefix, '_data.csv')
    df %>% write_csv(fn)
    print(str_c('saved file: ', fn))
  }
}
