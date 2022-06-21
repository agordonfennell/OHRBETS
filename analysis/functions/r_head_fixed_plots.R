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

plt_lick_mean_trial_lick_count_ring <- function(df, var_x, var_y, var_facet_y, var_facet_x, facet_scales, facet_spacing, scale_viridis_option, plt_manual_scale_y, plt_manual_scale_x, plt_manual_dims){

  plt <- df %>%
    ggplot(aes_string(var_x, var_y, color = var_x)) +
    geom_quasirandom(size = 2, shape = 21, fill = NA, alpha = 0.5, width = 0.3) +
    stat_summary(fun.data = 'mean_se', geom = 'errorbar', width = 0) +
    stat_summary(fun = 'mean', geom = 'hpline', aes(group = 1), size = 0.5, width = 0.5) +

    ylab('Mean Trial Lick Count') +

    scale_color_viridis_d(option = scale_viridis_option, end = 0.9) +
    theme_ag01() +
    theme(axis.text.x = element_blank(),
          axis.line.x = element_blank(),
          axis.ticks.x = element_blank(),
          axis.title.x = element_blank())


  plt <- plt %>%
    plt_string_facet(var_facet_y, var_facet_x, facet_scales, facet_spacing) %>%
    plt_manual_scale_cartesian(plt_manual_scale_x, plt_manual_scale_y) %>%
    plt_manual_dims(plt_manual_dims)


  return(plt)

}

plt_lick_mean_trial_lick_count_line <- function(df, var_x, var_y, var_facet_y, var_facet_x, facet_scales, facet_spacing, plt_manual_scale_y, plt_manual_scale_x, plt_manual_dims){

  plt <- df %>%
    ggplot(aes_string(var_x, var_y)) +
    geom_line(aes(group = subject), size = 1/4, alpha = 1/4) +
    stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = 1), width = 0) +
    stat_summary(fun = 'mean', geom = 'line', aes(group = 1), size = 0.25) +

    ylab('Mean Trial Lick Count') +
    xlab('Solution') +

    theme_ag01() +
    theme(axis.text.x = element_text(angle = 90, vjust = 0.3, hjust=1))


  plt <- plt %>%
    plt_string_facet(var_facet_y, var_facet_x, facet_scales, facet_spacing) %>%
    plt_manual_scale_cartesian(plt_manual_scale_x, plt_manual_scale_y) %>%
    plt_manual_dims(plt_manual_dims)


  return(plt)

}

plt_multi_spout_lick_summary_training <- function(data_spout_summary, limits_licks){

plt1 <- data_spout_summary %>%
  ggplot(aes(solution, lick_count_trial_mean)) +
  geom_line(aes(group = subject), alpha = 1/3)+
  stat_summary(fun = 'mean', geom = 'line', aes(group = 1)) +
  stat_summary(fun.data = 'mean_se', geom = 'errorbar', aes(group = 1), width = 0) +
  facet_grid(.~day) +
  theme_ag01() +
  ylab('Mean Trial Lick Count')

plt1 <- plt1 %>%
  remove_x_all()


plt1_mean <-
  data_spout_summary %>%
  group_by(subject, solution) %>%
  summarise(lick_count_trial_mean = lick_count_trial_mean %>% mean()) %>%
  plt_lick_mean_trial_lick_count_line(
    df = .,
    var_x = 'solution',
    var_y = 'lick_count_trial_mean',
    var_facet_y = NA,
    var_facet_x = NA,
    facet_scales = 'free',
    facet_spacing = 1,

    plt_manual_scale_y = c(0,15),
    plt_manual_scale_x = c(0,6),
    plt_manual_dims = c(3,2)
  )

plt1_mean <- plt1_mean %>%
  remove_x_all() %>%
  remove_y_all() +
  ggtitle('Mean')


plt2 <- data_spout_summary %>%
  ggplot(aes(solution, subject, fill = lick_count_trial_mean)) +
  geom_tile()+
  facet_grid(.~day) +
  theme_ag01() +
  scale_fill_continuous(low = 'black', high = 'white', limits = limits_licks, oob = scales::squish) +
  theme(axis.text.x = element_text(angle = 90, hjust = 1, vjust = 0.3)) +
  ylab('Subject')

plt2_mean <- data_spout_summary %>%
  group_by(subject, solution) %>%
  summarise(lick_count_trial_mean = lick_count_trial_mean %>% mean()) %>%
  ggplot(aes(solution, subject, fill = lick_count_trial_mean)) +
  geom_tile()+
  theme_ag01() +
  scale_fill_continuous(low = 'black', high = 'white', limits = limits_licks, oob = scales::squish) +
  theme(axis.text.x = element_text(angle = 90, hjust = 0, vjust = 0.3)) +
  ylab('Subject')

plt2_mean <- plt2_mean %>%
  remove_y_all()

  n_day <- data_spout_summary %>%
    select(day) %>% unique() %>%
    nrow()

plt_combined <- plt1 + plt1_mean + plt2 + plt2_mean +
  plot_layout(ncol = 2,
              widths  = unit(c(n_day * 2.5,2.5), rep('cm', 2)),
              heights = unit(rep(3, 2), rep('cm', 2)),
              guides = 'collect'
              )
return(plt_combined)
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

return_plt_spout_ids <- function(data_trial_summary){
  spout_ids <- data_trial_summary %>%
    select(spout, solution) %>%
    unique()

  plt_spout_ids <- spout_ids %>%
    gather('var', 'value', starts_with('lick_count')) %>%
    ggplot(aes(1, spout, fill = solution)) +
    geom_tile() +
    scale_fill_viridis_d(option = viridis_scale, end = 0.9) +
    ylab('Spout ID') +
    theme_ag01()

  plt_spout_ids <- plt_spout_ids %>%
    remove_x_all()

  return(plt_spout_ids)
}





