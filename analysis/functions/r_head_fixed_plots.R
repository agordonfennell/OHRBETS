# Title     : f_general_functions.R
# Objective : collect functions related to plotting in R including general asethetics and specific plot generation
# Created by: Adam Gordon-Fennell (agg2248@uw.edu)
#             Garret Stuber Lab, University of Washington
# Created on: 09/02/2020

# themes ---------------------------------------------------------------------------------------------------------------
theme_ag01 <- function () {
  aes.axis_line_size <- 0.25
  aes.axis_font_size <- 8
  aes.axis_title_font_size <- 8
  aes.title_font_size <- 8

  theme_bw(base_size=10) %+replace%
    theme(
      axis.line = element_line(colour = "black", size = aes.axis_line_size), # set x / y axis
      axis.ticks = element_line(size = aes.axis_line_size),
      axis.ticks.length = unit(1, "mm"),
      axis.title = element_text(color = "black", size = aes.axis_title_font_size),
      axis.text.x = element_text(color = "black", size = aes.axis_font_size, hjust = 0.5),
      axis.text.y = element_text(color = "black", size = aes.axis_font_size, hjust = 1, vjust = 0.3, margin = margin(r = 1)),
      legend.title = element_text(color = "black", size = aes.axis_title_font_size),
      legend.text = element_text(color = "black", size = aes.axis_font_size),
      plot.title = element_text(color = "black", size = aes.title_font_size, hjust = 0.5),
      panel.grid.major = element_blank(), # turn off major grid
      panel.grid.minor = element_blank(), # turn off minor grid
      panel.border = element_blank(),     # turn of background panel border
      panel.background = element_blank(),
      strip.background = element_blank(),
      strip.text = element_text(color = 'black', size = aes.axis_title_font_size)
    )
}


theme_ag_raster <- function () {
  aes.axis_line_size <- 0.25
  aes.axis_font_size <- 8
  aes.axis_title_font_size <- 8
  aes.title_font_size <- 8

  theme_bw(base_size=10) %+replace%
    theme(
      axis.ticks = element_line(size = aes.axis_line_size),
      axis.title = element_text(color = "black", size = aes.axis_title_font_size),
      legend.title = element_text(color = "black", size = aes.axis_title_font_size),
      legend.text = element_text(color = "black", size = aes.axis_font_size),
      plot.title = element_text(color = "black", size = aes.title_font_size, hjust = 0.5, face="bold"),
      panel.border = element_blank(),     # turn of background panel border
      panel.background = element_blank(),
      strip.background = element_blank(),
      strip.text = element_text(color = 'black', size = aes.axis_title_font_size),
      panel.grid.minor.y=element_blank(),
      panel.grid.major.y=element_blank(),
      axis.line.x = element_blank(),
      axis.title.x=element_blank(),
      axis.text.x=element_blank(),
      axis.ticks.x=element_blank()
    )
}

# misc -----------------------------------------------------------------------------------------------------------------
remove_x_all <- function(plt){
  plt +
    theme(axis.text.x = element_blank(),
        axis.line.x = element_blank(),
        axis.ticks.x = element_blank(),
        axis.title.x = element_blank())

}

remove_y_all <- function(plt){
  plt +
    theme(axis.text.y = element_blank(),
        axis.line.y = element_blank(),
        axis.ticks.y = element_blank(),
        axis.title.y = element_blank())

}

# geoms ----------------------------------------------------------------------------------------------------------------
# source: wilkelab/ungeviz
geom_hpline <- function(mapping = NULL, data = NULL,
                        stat = "identity", position = "identity",
                        ...,
                        na.rm = FALSE,
                        show.legend = NA,
                        inherit.aes = TRUE) {
  layer(
    data = data,
    mapping = mapping,
    stat = stat,
    geom = GeomHpline,
    position = position,
    show.legend = show.legend,
    inherit.aes = inherit.aes,
    params = list(
      na.rm = na.rm,
      ...
    )
  )
}

# source: wilkelab/ungeviz
GeomHpline <- ggproto("GeomHpline", GeomSegment,
  required_aes = c("x", "y"),
  non_missing_aes = c("size", "colour", "linetype", "width"),
  default_aes = aes(
    width = 0.5, colour = "black", size = 2, linetype = 1,
    alpha = NA
  ),

  draw_panel = function(self, data, panel_params, coord, arrow = NULL, arrow.fill = NULL,
                        lineend = "butt", linejoin = "round", na.rm = FALSE) {
    data <- mutate(data, x = x - width/2, xend = x + width, yend = y)
    ggproto_parent(GeomSegment, self)$draw_panel(
      data, panel_params, coord, arrow = arrow, arrow.fill = arrow.fill,
      lineend = lineend, linejoin = linejoin, na.rm = na.rm
    )
  }
)

# saving ---------------------------------------------------------------------------------------------------------------
save_pdf <- function(dir, fn, w, h){
  ggsave(
    filename =  str_c(dir, fn),
    device = NULL,
    path = NULL,
    scale = 1,
    width = w,
    height = h,
    units = c("in"),
    dpi = 300,
    useDingbats=FALSE
  )
}

# general functions ----------------------------------------------------------------------------------------------------
plt_string_facet <- function(plt, var_facet_y, var_facet_x, scales, facet_spacing){
  # returns plot faceted based on strings for y and x faceting variables
  # input NA in place of strings to avoid faceting

  if(!is.na(var_facet_y) & !is.na(var_facet_x)){
    plt <- plt + facet_grid(as.formula(str_c(var_facet_y, '~', var_facet_x)), scales = scales) +
      theme(panel.spacing = unit(facet_spacing, "lines"))
  }

  if(!is.na(var_facet_y) & is.na(var_facet_x)){
    plt <- plt + facet_grid(as.formula(str_c(var_facet_y, '~.')), scales = scales) +
      theme(panel.spacing = unit(facet_spacing, "lines"))
  }

  if(is.na(var_facet_y) & !is.na(var_facet_x)){
    plt <- plt + facet_grid(as.formula(str_c('.~', var_facet_x)), scales = scales) +
      theme(panel.spacing = unit(facet_spacing, "lines"))
  }

  return(plt)
}

plt_manual_dims <- function(plt, plt_dims){
  if(length(plt_dims) > 1){
    plt <- plt +
    force_panelsizes(rows = unit(plt_dims[1], "cm"),
                        cols = unit(plt_dims[2], "cm"))
  }

  return(plt)
}

plt_manual_scale_cartesian <- function(plt, plt_manual_scale_x, plt_manual_scale_y){
  # manually scale axis

  if(sum(is.na(plt_manual_scale_x) == 0) & sum(is.na(plt_manual_scale_y) == 0)){
    plt <- plt +
      coord_cartesian(xlim = plt_manual_scale_x, ylim = plt_manual_scale_y, expand = FALSE, clip = 'off')
  }

  if(sum(is.na(plt_manual_scale_x) == 0) & !sum(is.na(plt_manual_scale_y) == 0)){
    plt <- plt +
      coord_cartesian(xlim = plt_manual_scale_x, expand = FALSE, clip = 'off')
  }

  if(!sum(is.na(plt_manual_scale_x) == 0) & sum(is.na(plt_manual_scale_y) == 0)){
    plt <- plt +
      coord_cartesian(ylim = plt_manual_scale_y, expand = FALSE, clip = 'off')
  }

  return(plt)
}


# universal plots ----------------------------------------------------------------------------------------------------
plt_raster <- function(df, x, y, id, facet_x, facet_y, xlab, ylab, plt_y_lims, plt_x_lims, plt_dims, var_color, plt_scale_color){

  plt <- df %>%
    ungroup() %>%
    mutate(event_unique = row_number()) %>%
    select(na.omit(c(facet_y, facet_x, x, var_color, 'event_unique', y))) %>%
    mutate(y_dummy = !!as.name(y) + 1) %>%
    gather('id', 'y_pos', !!as.name(y):y_dummy) %>%

    ggplot(aes(event_ts_rel, y_pos, group = event_unique))

  if(is.na(var_color)) {plt <- plt + geom_line(size = 0.5)}
  if(!is.na(var_color)){plt <- plt + geom_line(size = 0.5, aes(color = !!as.name(var_color)))}


  if(!is.na(plt_scale_color) & !is.na(var_color)){
    if(plt_scale_color == 'mako'){plt <- plt + scale_color_viridis_d(option = 'mako', end = 0.9)}
    if(plt_scale_color == 'inferno'){plt <- plt + scale_color_viridis_d(option = 'inferno', end = 0.9)}
    if(plt_scale_color == 'viridis'){plt <- plt + scale_color_viridis_d(option = 'viridis', end = 0.9)}
  }

  if(!is.na(facet_x) & !is.na(facet_y)){plt <- plt + facet_grid(eval(expr(!!ensym(facet_y) ~ !!ensym(facet_x))))}
  if(!is.na(facet_x) &  is.na(facet_y)){plt <- plt + facet_grid(eval(expr(. ~ !!ensym(facet_x))))}
  if( is.na(facet_x) & !is.na(facet_y)){plt <- plt + facet_grid(eval(expr(!!ensym(facet_y) ~ .)))}

  plt <- plt +
    theme_ag01() +
    coord_cartesian(ylim = plt_y_lims, xlim = plt_x_lims, expand = FALSE) +
    scale_y_continuous(breaks = plt_y_lims) +
    scale_x_continuous(breaks = plt_x_lims) +
    theme(panel.spacing = unit(0.5, "lines")) +
    xlab(xlab) +
    ylab(ylab)

  if(sum(!is.na(plt_dims) > 0)){
      plt <- plt +
        force_panelsizes(rows = unit(plt_dims[1], "cm"),
                     cols = unit(plt_dims[2], "cm"))
  }

  plt
}

plt_heatmap_trial_split <- function(df, var_x, var_fill, var_facet, var_trial, limits_fill, bin_seq, plt_dim){
  # return heat plot of mean values over binned trials

  # setup split data
  df_split <- df %>%
    mutate(trial_split = cut(!!as.name(var_trial), bin_seq, label = bin_seq[1:length(bin_seq)-1])) %>%
    group_by_at(na.omit(c('trial_split', var_x, var_facet))) %>%
    summarise_at(vars(var_fill), mean) %>%
    mutate(trial_split = trial_split %>% as.character() %>% as.double()) %>%
    ungroup() %>%
    mutate(trial_split = trial_split + (bin_seq[2]/2)) %>%
    filter(!is.na(!!as.name(var_x)))

  # save solution count
  var_x_count <- df_split %>% select(!!as.name(var_x)) %>% unique() %>% nrow()

  plt <- df_split %>%
   ggplot(aes_string(x = var_x, y = 'trial_split', fill = var_fill)) +
   geom_tile() +
   theme_ag01() +
   coord_cartesian(ylim = c(0,max(bin_seq)), xlim = c(0.5, var_x_count + 0.5), expand = FALSE) +
   theme(axis.text.x = element_text(angle = 90, vjust = 0.3, hjust=1)) +
   ylab('Trial Bin')

  # apply limits to plot
  if(sum(!is.na(limits_fill)) == 0){
   plt <- plt + scale_fill_continuous(low = 'black', high = 'white', oob = scales::squish)
  } else {
   plt <- plt + scale_fill_continuous(low = 'black', high = 'white', limits = limits_fill, oob = scales::squish)
  }

  # facet plot
  if(sum(!is.na(var_facet)) > 0){
   plt <- plt + facet_grid(as.formula(paste(".~", var_facet)))
  }
  plt <- plt %>%
    plt_manual_dims(plt_dim)

  #return plot
  return(plt)
}


plt_mean_trial_split <- function(df, var_y, var_facet, var_trial, limits_fill, bin_seq, plt_dim){
  # return heat plot of mean values over binned trials

  # setup split data
  df_split <- df %>%
    mutate(trial_split = cut(!!as.name(var_trial), bin_seq, label = bin_seq[1:length(bin_seq)-1])) %>%
    group_by_at(na.omit(c('trial_split', 'subject', var_facet))) %>%
    summarise_at(vars(var_y), mean) %>%
    mutate(trial_split = trial_split %>% as.character() %>% as.double()) %>%
    ungroup() %>%
    mutate(trial_split = trial_split + (bin_seq[2]/2))


  # save solution count

  plt <- df_split %>%
   ggplot(aes_string(x = 'trial_split', y = var_y)) +
   geom_line(alpha = 1/3, size = 0.25, aes(group = subject)) +
   stat_summary(fun = 'mean', geom = 'line', aes(group = 1), size = 0.25) +
   stat_summary(fun.data = 'mean_se', geom = 'errorbar', width = 0, aes(group = 1), size = 0.25) +
   theme_ag01() +
   coord_cartesian(xlim = c(0,max(bin_seq)), expand = FALSE) +
   xlab('Trial Bin')

  # apply limits to plot
  if(sum(!is.na(limits_fill)) == 0){
   plt <- plt + scale_fill_continuous(low = 'black', high = 'white', oob = scales::squish)
  } else {
   plt <- plt + scale_fill_continuous(low = 'black', high = 'white', limits = limits_fill, oob = scales::squish)
  }

  # facet plot
  if(sum(!is.na(var_facet)) > 0){
   plt <- plt + facet_grid(as.formula(paste(".~", var_facet)))
  }
  plt <- plt %>%
    plt_manual_dims(plt_dim)

  #return plot
  return(plt)
}

# head-fixed multi-spout brief access ----------------------------------------------------------------------------------


plt_binned_lick_rate <- function(df, facet_x, facet_y, xlab, ylab, plt_y_lims, plt_x_lims, plt_dims, viridis_scale){


  plt <- df %>%
    filter(time_bin >=plt_x_lims[1], time_bin < plt_x_lims[2]) %>%
    mutate(binned_freq = count_binned / (time_bin_width/1000)) %>%
    ggplot(aes(time_bin, binned_freq, color = solution, fill = solution)) +
    stat_summary(fun.data = 'mean_se', geom = 'ribbon', aes(group = solution), alpha = 1/3, color = NA) +
    stat_summary(fun = 'mean', geom = 'line', aes(group = solution)) +
    scale_color_viridis_d(option = viridis_scale, end = 0.9) +
    scale_fill_viridis_d(option = viridis_scale, end = 0.9) +
    coord_cartesian( xlim = c(0, plt_x_lims[2]), expand = FALSE, clip = 'off') +
    scale_x_continuous(breaks = c(0,plt_x_lims[2]))



  if(!is.na(facet_x) & !is.na(facet_y)){plt <- plt + facet_grid(eval(expr(!!ensym(facet_y) ~ !!ensym(facet_x))))}
  if(!is.na(facet_x) &  is.na(facet_y)){plt <- plt + facet_grid(eval(expr(. ~ !!ensym(facet_x))))}
  if( is.na(facet_x) & !is.na(facet_y)){plt <- plt + facet_grid(eval(expr(!!ensym(facet_y) ~ .)))}

  plt <- plt +
    theme_ag01() +
    theme(panel.spacing = unit(0.5, "lines")) +
    xlab(xlab) +
    ylab(ylab)


  if(sum(is.na(plt_x_lims)) == 0 & sum(is.na(plt_y_lims)) == 0 ){plt <- plt + coord_cartesian(ylim = plt_y_lims, xlim = plt_x_lims, expand = FALSE)}
  if(sum(is.na(plt_x_lims)) >  0 & sum(is.na(plt_y_lims)) == 0 ){plt <- plt + coord_cartesian(ylim = plt_y_lims, expand = FALSE)}
  if(sum(is.na(plt_x_lims)) == 0 & sum(is.na(plt_y_lims)) >  0 ){plt <- plt + coord_cartesian(xlim = plt_x_lims, expand = FALSE)}
  if(sum(is.na(plt_x_lims)) >  0 & sum(is.na(plt_y_lims)) >  0 ){plt <- plt + coord_cartesian(expand = FALSE)}


  if(sum(!is.na(plt_dims) > 0)){
      plt <- plt +
        force_panelsizes(rows = unit(plt_dims[1], "cm"),
                     cols = unit(plt_dims[2], "cm"))
  }

  plt
}


return_viridis_scale <- function(df){
  session_solutions <- df %>%
    pull(solution) %>%
    unique()

  viridis_scale <- NA

  if(sum(session_solutions %>% str_detect('nacl')) == length(session_solutions)){
    viridis_scale = 'inferno'
  }

  if(sum(session_solutions %>% str_detect('sucrose')) == length(session_solutions)){
    viridis_scale = 'mako'
  }

  if(sum(session_solutions %>% str_detect('quinine')) == length(session_solutions)){
    viridis_scale = 'viridis'
  }
  return(viridis_scale)
}

return_plt_spout_ids <- function(data_trial_summary, var_session, plotting_multi_sbj, viridis_scale){

  if(plotting_multi_sbj == 0){
    spout_ids <- data_trial_summary %>%
      select(spout, solution) %>%
      unique()

    plt_spout_ids <- spout_ids %>%
      ggplot(aes(1, spout, fill = solution)) +
      geom_tile() +
      scale_fill_viridis_d(option = viridis_scale, end = 0.9) +
      ylab('Spout ID') +
      theme_ag01()

      plt_spout_ids <- plt_spout_ids %>%
      remove_x_all()
  }

  if(plotting_multi_sbj == 1){
    spout_ids <- data_trial_summary %>%
      select(all_of(c('spout', 'solution', var_session))) %>%
      unique()

    plt_spout_ids <- spout_ids %>%
      ggplot(aes_string(var_session, 'spout', fill = 'solution')) +
      geom_tile() +
      scale_fill_viridis_d(option = viridis_scale, end = 0.9) +
      ylab('Spout ID') +
      theme_ag01()
  }




  return(plt_spout_ids)
}

combined_plt_multi_spout_summary <- function(dir_extracted, analysis_fns, log_data, analysis_id, dir_output, file_format){

  first_loop <- 1

  for(blockname in analysis_fns){
    if(str_detect(file_format, 'feather')){
      session_parameters_loop <- read_feather(str_c(dir_extracted, blockname, '_param.', file_format))
      data_trial_loop <- read_feather(str_c(dir_processed, blockname, '_data_trial.', file_format))
      data_trial_summary_loop <- read_feather(str_c(dir_processed, blockname, '_data_trial_summary.', file_format))
      data_trial_binned_loop <- read_feather(str_c(dir_processed, blockname, '_data_trial_binned.', file_format))
      data_session_binned_spout_loop <- read_feather(str_c(dir_processed, blockname, '_data_session_binned_spout.', file_format))
    } else if(str_detect(file_format, 'csv')){
      session_parameters_loop <- read_csv(str_c(dir_extracted, blockname, '_param.', file_format), col_types = cols())
      data_trial_loop <- read_csv(str_c(dir_processed, blockname, '_data_trial.', file_format), col_types = cols())
      data_trial_summary_loop <- read_csv(str_c(dir_processed, blockname, '_data_trial_summary.', file_format), col_types = cols())
      data_trial_binned_loop <- read_csv(str_c(dir_processed, blockname, '_data_trial_binned.', file_format), col_types = cols())
      data_session_binned_spout_loop <- read_csv(str_c(dir_processed, blockname, '_data_session_binned_spout.', file_format), col_types = cols())
    } else {
      print('Error: incompatable file type (requires .feather or .csv)')
      return(0)
    }

    if(first_loop){
      session_parameters <- session_parameters_loop
      data_trial <- data_trial_loop
      data_trial_summary <- data_trial_summary_loop
      data_trial_binned <- data_trial_binned_loop
      data_session_binned_spout <- data_session_binned_spout_loop
      first_loop <- 0
    } else {
      session_parameters <- session_parameters_loop %>% bind_rows(session_parameters,.)
      data_trial <- data_trial_loop %>% bind_rows(data_trial,.)
      data_trial_summary <- data_trial_summary_loop %>% bind_rows(data_trial_summary,.)
      data_trial_binned <- data_trial_binned_loop %>% bind_rows(data_trial_binned,.)
      data_session_binned_spout <- data_session_binned_spout_loop %>% bind_rows(data_session_binned_spout,.)
    }
  }

  # join info
  join_log_data <- function(df, log_data, vars_to_join, vars_by){
    log_data <- log_data %>%
      select(all_of(c(vars_to_join, vars_by)))

    df %>%
      left_join(log_data, by = vars_by)
  }

  vars_to_join <- c('subject', 'procedure', 'day')
  vars_by <- c('blockname')

  session_parameters <- session_parameters %>% join_log_data(log_data, vars_to_join, vars_by)
  data_trial <- data_trial %>% join_log_data(log_data, vars_to_join, vars_by)
  data_trial_summary <- data_trial_summary %>% join_log_data(log_data, vars_to_join, vars_by)
  data_trial_binned <- data_trial_binned %>% join_log_data(log_data, vars_to_join, vars_by)
  data_session_binned_spout <- data_session_binned_spout %>% join_log_data(log_data, vars_to_join, vars_by)


  data_lick_proportion <- data_trial_summary %>%
      group_by(solution, subject, blockname) %>%
      summarise(lick_proportion = sum(trial_lick) / max(trial_num_tastant)) %>%
      group_by(solution, subject) %>% summarise(lick_proportion = mean(lick_proportion))

  data_session_summary <- data_trial_summary %>%
    group_by(subject, solution) %>%
    summarise(lick_count_mean = lick_count %>% mean(),
              lick_count_total = lick_count %>% sum())

  plt_combined <- plt_multi_spout_summary(
    session_parameters,
    data_trial,
    data_trial_summary,
    data_trial_binned,
    data_session_binned_spout,
    data_lick_proportion,
    data_session_summary,
    plotting_multi_sbj = 1,
    analysis_id =  analysis_id,
    dir_output = dir_output,
    blockname = NA
    )

    print('saving data')
    session_parameters %>% write_csv(str_c(dir_output, analysis_id, '_session_parameters.csv'))
    data_trial_summary %>% write_csv(str_c(dir_output, analysis_id, '_data_trial_summary.csv'))
    data_trial_binned %>% write_csv(str_c(dir_output, analysis_id, '_data_trial_binned.csv'))
    data_session_binned_spout %>% write_csv(str_c(dir_output, analysis_id, '_data_session_binned_spout.csv'))
    data_lick_proportion %>% write_csv(str_c(dir_output, analysis_id, '_data_lick_proportion.csv'))
    data_session_summary %>% write_csv(str_c(dir_output, analysis_id, '_data_session_summary.csv'))

    print('saving stats')
    stats_results <- data_session_summary %>%
      aov_rm_one_within(., dir_output, str_c(analysis_id, '_stats_triallickcount'), 'subject', 'lick_count_mean', 'solution')

    stats_results <- data_lick_proportion %>%
      aov_rm_one_within(., dir_output, str_c(analysis_id, '_stats_triallickproportion'), 'subject', 'lick_proportion', 'solution')

    return(plt_combined)
}

batch_session_plt_multi_spout_summary <- function(dir_processed, dir_extracted, file_format = 'csv', manual_blocknames = NA, overwrite = 0){

  fns_dir_processed <- list.files(dir_processed)

  data_suffixes <- c(
    '_data_session_binned.',
    '_data_session_binned_spout.',
    '_data_session.',
    '_data_spout_summary.',
    '_data_trial_binned.',
    '_data_trial_summary.',
    '_data_trial.'
  )

  for(suffix in data_suffixes){
    fns_dir_processed <- fns_dir_processed %>% str_remove(str_c(suffix, file_format))
  }

  plt_suffixes <- c(
    '_plt_session_summary.pdf',
    '_plt_session_summary.png'
  )

  for(suffix in plt_suffixes){
    fns_dir_processed <- fns_dir_processed %>% str_remove(str_c(suffix))
  }

  blocknames <- fns_dir_processed %>% unique()

  # use manual_fns to filter to predetermined set
  if(sum(!is.na(manual_blocknames)) > 0){
    blocknames <- blocknames[blocknames %in% manual_blocknames]
  }

  # remove blocknames already processed
  if(!overwrite){
    fns_dir_plt <- list.files(dir_processed)
    fns_dir_plt <- fns_dir_plt[fns_dir_plt %>% str_detect('.pdf') | fns_dir_plt %>% str_detect('.png')]

    for(suffix in plt_suffixes){
      fns_dir_plt <- fns_dir_plt %>% str_remove(str_c(suffix))
    }

    fns_dir_plt <- fns_dir_plt %>% unique()

    for(fn in fns_dir_plt){
      blocknames <- blocknames[!blocknames %>% str_detect(fn)]
    }

    if(length(blocknames) == 0){
      print(str_c('all data located in dir ', dir_processed, ' have already been plotted'))
      return(0)
      }

  }

  for(blockname in blocknames){

   if(str_detect(file_format, 'feather')){
     session_parameters <- read_feather(str_c(dir_extracted, blockname, '_param.', file_format))
     data_trial <- read_feather(str_c(dir_processed, blockname, '_data_trial.', file_format))
     data_trial_summary <- read_feather(str_c(dir_processed, blockname, '_data_trial_summary.', file_format))
     data_trial_binned <- read_feather(str_c(dir_processed, blockname, '_data_trial_binned.', file_format))
     data_session_binned_spout <- read_feather(str_c(dir_processed, blockname, '_data_session_binned_spout.', file_format))
   } else if(str_detect(file_format, 'csv')){
     session_parameters <- read_csv(str_c(dir_extracted, blockname, '_param.', file_format),col_types = cols())
     data_trial <- read_csv(str_c(dir_processed, blockname, '_data_trial.', file_format), col_types = cols())
     data_trial_summary <- read_csv(str_c(dir_processed, blockname, '_data_trial_summary.', file_format), col_types = cols())
     data_trial_binned <- read_csv(str_c(dir_processed, blockname, '_data_trial_binned.', file_format), col_types = cols())
     data_session_binned_spout <- read_csv(str_c(dir_processed, blockname, '_data_session_binned_spout.', file_format), col_types = cols())
  } else {
     print('Error: incompatable file type (requires .feather or .csv)')
   }

    plt_multi_spout_summary(
      session_parameters,
      data_trial,
      data_trial_summary,
      data_trial_binned,
      data_session_binned_spout,
      NA,
      NA,
      plotting_multi_sbj = 0,
      analysis_id = NA,
      dir_output = dir_processed,
      blockname = blockname
    )
  }
}

plt_multi_spout_summary <- function(session_parameters, data_trial, data_trial_summary, data_trial_binned, data_session_binned_spout, data_lick_proportion,
    data_session_summary, plotting_multi_sbj, analysis_id, dir_output, blockname){
  # return color scale based on solutions
  viridis_scale <- return_viridis_scale(data_trial)

  # return xlims for trial time series plots
  xlim_max <- session_parameters %>%
    pull(access_time) %>%
    unique()

  if(length(xlim_max) > 1){
    print('Error: multiple access times used in data included for combined plotting')
    return(0)
  }

  trial_count <- data_trial_summary %>%
    group_by(blockname) %>%
    filter(trial_num == max(trial_num)) %>%
    ungroup() %>%
    select(trial_num) %>%
    unique() %>%
    pull()

  if(length(xlim_max) > 1){
    print('Error: multiple trial lengths in data included for combined plotting')
    return(0)
  }


  if(plotting_multi_sbj == 0){
    plt_lickraster <- data_trial %>%
      mutate(lick_number = row_number()) %>%
      group_by(solution) %>%
      plt_raster(.,
                 x = 'event_ts_rel',
                 y = 'trial_num',
                 id = 'subject',
                 facet_x = NA,
                 facet_y = NA,
                 xlab = 'Time (ms)',
                 ylab = 'Trial Number',
                 plt_y_lims = c(0, trial_count),
                 plt_x_lims = c(0, xlim_max),
                 plt_dims = NA,
                 var_color = 'solution',
                 plt_scale_color = viridis_scale)

    plt_lickraster_sorted <- data_trial %>%
      mutate(lick_number = row_number()) %>%
      group_by(solution) %>%
      plt_raster(.,
                 x = 'event_ts_rel',
                 y = 'trial_num_tastant',
                 id = 'subject',
                 facet_x = NA,
                 facet_y = 'solution',
                 xlab = 'Time (ms)',
                 ylab = 'Sorted Trial Number',
                 plt_y_lims = c(0, data_trial %>% select(trial_num_tastant) %>% pull() %>%  max()),
                 plt_x_lims = c(0,xlim_max),
                 plt_dims = NA,
                 var_color = 'solution',
                 plt_scale_color = viridis_scale) +
       theme(strip.text.y = element_blank())
  }

  # return plot for spout ids
  plt_spout_ids <- return_plt_spout_ids(data_trial_summary, var_session = 'day', plotting_multi_sbj, viridis_scale)

  # return plot for binned lick rate
  if(plotting_multi_sbj == 0){
    plt03 <- data_trial_binned %>%
      plt_binned_lick_rate(
        df = .,
        facet_x = NA,
        facet_y = NA,
        xlab = 'Time (ms)',
        ylab = 'Lick Rate (Hz)',
        plt_y_lims = NA,
        plt_x_lims = c(0, 3000),
        plt_dims = NA,
        viridis_scale = viridis_scale
      )
  }

  if(plotting_multi_sbj == 1){
    plt03 <- data_trial_binned %>%
      group_by(subject, solution, time_bin_width, time_bin) %>%
      summarise(count_binned = count_binned %>% mean()) %>%
      plt_binned_lick_rate(
        df = .,
        facet_x = NA,
        facet_y = NA,
        xlab = 'Time (ms)',
        ylab = 'Lick Rate (Hz)',
        plt_y_lims = NA,
        plt_x_lims = c(0, 3000),
        plt_dims = NA,
        viridis_scale = viridis_scale
      )
  }

plt04 <- data_trial_summary  %>%
  filter(lick_count > 0) %>%
  ggplot(aes(lick_count, color = solution)) +
  stat_ecdf() +
  scale_color_viridis_d(option = viridis_scale, end = 0.9) +
  theme_ag01() +
  coord_cartesian(ylim = c(0,1), xlim = c(0, max(data_trial_summary$lick_count)), expand = FALSE) +
  scale_y_continuous(breaks = c(0,1)) +
  scale_x_continuous(breaks = c(0,max(data_trial_summary$lick_count))) +
  ggtitle('Lick Count > 0 \n') +
  ylab('Proportion') +
  theme(axis.title.x = element_blank())

plt05 <- data_trial_summary  %>%
  filter(lick_count > 0) %>%
  ggplot(aes(lick_ts_last, color = solution)) +
  stat_ecdf() +
  scale_color_viridis_d(option = viridis_scale, end = 0.9) +
  theme_ag01() +
  coord_cartesian(ylim = c(0,1), xlim = c(0, xlim_max), expand = FALSE) +
  scale_y_continuous(breaks = c(0,1)) +
  scale_x_continuous(breaks = c(0,xlim_max)) +
  ggtitle('Last Lick (ms) \n') +
  theme(axis.title.x = element_blank())

plt05 <- remove_y_all(plt05)

if(plotting_multi_sbj == 0){
  plt06 <- data_trial_summary %>%
    ggplot(aes(solution, lick_count, color = solution)) +
    geom_beeswarm(cex = 3, alpha = 1/3, stroke = 0.2, shape = 21, size = 3, fill = NA) +
    stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = 1), size = 0.5, width = 0) +
    stat_summary(fun = 'mean',         geom = 'hpline', aes(group = 1), size = 0.5, width = 0.8) +
    theme_ag01() +
    scale_color_viridis_d(option = viridis_scale, end = 0.9) +
    xlab('Solution') +
    ylab('Trial Lick Count')
}

if(plotting_multi_sbj == 1){
  plt06 <- data_session_summary %>%
    ggplot(aes(solution, lick_count_mean, color = solution)) +
    geom_beeswarm(cex = 3, alpha = 1/3, stroke = 0.2, shape = 21, size = 3, fill = NA) +
    stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = 1), size = 0.5, width = 0) +
    stat_summary(fun = 'mean',         geom = 'hpline', aes(group = 1), size = 0.5, width = 0.8) +
    theme_ag01() +
    scale_color_viridis_d(option = viridis_scale, end = 0.9) +
    xlab('Solution') +
    ylab('Trial Lick Count')
}

  plt06 <- plt06 %>% remove_x_all()

# split into single subject vs multi subject
  if(plotting_multi_sbj == 0){
    plt07 <- data_trial_summary %>%
      group_by(solution) %>% # single subject
      summarise(lick_proportion = sum(trial_lick) / max(trial_num_tastant)) %>%   # multi subject
      ggplot(aes(solution, lick_proportion, color = solution)) +
      geom_hpline(size = 0.5, width = 0.8) +
      theme_ag01() +
      coord_cartesian(ylim = c(0,1), xlim = c(0, length(unique(data_trial_summary$solution)) + 1), expand = FALSE, clip = 'off') +
      scale_color_viridis_d(option = viridis_scale, end = 0.9) +
      xlab('Solution') +
      ylab('Trial Lick Proportion')
  }

  if(plotting_multi_sbj == 1){
    plt07 <- data_lick_proportion %>%
      ggplot(aes(solution, lick_proportion, color = solution)) +
      geom_beeswarm(cex = 3, alpha = 1/3, stroke = 0.2, shape = 21, size = 3, fill = NA) +
      stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = 1), size = 0.5, width = 0) +
      stat_summary(fun = 'mean',         geom = 'hpline', aes(group = 1), size = 0.5, width = 0.8) +
      theme_ag01() +
      coord_cartesian(ylim = c(0,1), xlim = c(0, length(unique(data_trial_summary$solution)) + 1), expand = FALSE, clip = 'off') +
      scale_color_viridis_d(option = viridis_scale, end = 0.9) +
      xlab('Solution') +
      ylab('Trial Lick Proportion')
  }

  plt07 <- plt07 %>% remove_x_all()


  if(plotting_multi_sbj == 0){
    plt08 <- data_session_binned_spout %>%
      ggplot(aes(trial_split + 5, lick_count_mean, color = solution)) +
      stat_summary(fun = 'mean', geom = 'line', aes(group = 1), color = 'darkgrey', linetype = 2) +
      geom_line(aes(group = solution)) +
      theme_ag01() +
      coord_cartesian( xlim = c(0,trial_count), expand = FALSE, clip = 'off') +
      scale_color_viridis_d(option = viridis_scale, end = 0.9) +
      ylab('Mean Lick Count') +
      xlab('Trial Bin')
  }

  if(plotting_multi_sbj == 1){
    plt08 <- data_session_binned_spout %>%
      group_by(subject, solution, trial_split) %>%
      summarise(lick_count_mean = lick_count_mean %>% mean()) %>%
      ggplot(aes(trial_split + 5, lick_count_mean, color = solution)) +
      stat_summary(fun = 'mean', geom = 'line', aes(group = solution)) +
      stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = solution), width = 0) +
      theme_ag01() +
      coord_cartesian( xlim = c(0,trial_count), expand = FALSE, clip = 'off') +
      scale_color_viridis_d(option = viridis_scale, end = 0.9) +
      ylab('Mean Lick Count') +
      xlab('Trial Bin')
  }

  plt09 <-  plt_heatmap_trial_split(df = data_trial_summary,
                            var_x = 'solution',
                            var_fill = 'lick_count',
                            var_facet = NA,
                            var_trial = 'trial_num',
                            limits_fill = NA,
                            bin_seq = seq(0, trial_count, 10),
                            plt_dim = NA)

  plt_ili <- data_trial %>%
    arrange(blockname, trial_num, event_ts_rel) %>%
    group_by(blockname, trial_num) %>%
    mutate(ili = event_ts_rel - lag(event_ts_rel)) %>%
    filter(ili < 1000) %>%
    ggplot(aes(ili, color = solution)) +
    geom_density(adjust = 1.3, size = 0.5) +

    scale_color_viridis_d(option = viridis_scale, end = 0.9) +

    theme_ag01() +
    coord_cartesian(xlim = c(0, 400), expand = FALSE) +
    xlab('Inter Lick Interval (ms)') +
    ylab('Density')

  # adjust plot aesthetics
  plt_ili <- plt_ili + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
  plt_spout_ids <- plt_spout_ids + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
  plt03 <- plt03 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm"))
  plt04 <- plt04 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
  plt05 <- plt05 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
  plt06 <- plt06 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0, "cm")) + theme(legend.position = "none")
  plt07 <- plt07 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0, "cm")) + theme(legend.position = "none")
  plt08 <- plt08 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
  plt09 <- plt09 + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm"))

  if(plotting_multi_sbj == 0){
    plt_lickraster <- plt_lickraster + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")
    plt_lickraster_sorted <- plt_lickraster_sorted + theme(plot.margin = margin(0.5, 0.5, 0.5, 0.5, "cm")) + theme(legend.position = "none")

    combined_plt <- plt_spout_ids + plt_lickraster + plt_lickraster_sorted + plt_ili + plt03 + plt04 + plt05 + plt06 + plt07 + plt08 + plt09 +
       plot_layout(widths = unit(c(1, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5), c('cm','cm','cm')),
                   heights = unit(c(5), c('cm')),
                   guides = 'collect') +
      plot_annotation(title = blockname)

    print('saving plots: ')
    print(str_c('~', dir_output, blockname, '_plt_session_summary.pdf'))
    print(str_c('~', dir_output, blockname, '_plt_session_summary.png'))

    ggsave(
      filename = str_c(dir_output, blockname, '_plt_session_summary.pdf'),
      device = NULL,
      path = NULL,
      scale = 1,
      width = 20,
      height = 4,
      units = c("in"),
      dpi = 300,
      useDingbats=FALSE
    )

    ggsave(
      filename = str_c(dir_output, blockname, '_plt_session_summary.png'),
      device = NULL,
      path = NULL,
      scale = 1,
      width = 20,
      height = 4,
      units = c("in"),
      dpi = 300
    )
  }

  if(plotting_multi_sbj == 1){
    combined_plt <- plt_spout_ids + plt_ili + plt03 + plt04 + plt05 + plt06 + plt07 + plt08 + plt09 +
       plot_layout(widths = unit(c(2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5, 2.5), c('cm','cm','cm')),
                   heights = unit(c(5), c('cm')),
                   guides = 'collect') +
      plot_annotation(title = analysis_id)

    print('saving plots: ')
    print(str_c('~', dir_output, analysis_id, '_plt_session_summary.pdf'))
    print(str_c('~', dir_output, analysis_id, '_plt_session_summary.png'))

    ggsave(
      filename = str_c(dir_output, analysis_id, '_plt_session_summary.pdf'),
      device = NULL,
      path = NULL,
      scale = 1,
      width = 20,
      height = 4,
      units = c("in"),
      dpi = 300,
      useDingbats=FALSE
    )

    ggsave(
      filename = str_c(dir_output, analysis_id, '_plt_session_summary.png'),
      device = NULL,
      path = NULL,
      scale = 1,
      width = 20,
      height = 4,
      units = c("in"),
      dpi = 300
    )
  }

  return(combined_plt)
}

