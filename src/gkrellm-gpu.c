/* GKrellM GPU Plugin for NVIDIA GPUs
 * 
 * Displays GPU utilization and VRAM usage similar to nvtop
 * Uses NVIDIA Management Library (NVML) for data collection
 */

#include <gkrellm2/gkrellm.h>
#include <nvml.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define CONFIG_NAME    "GPU"
#define STYLE_NAME     "gpu"
#define CFG_BUFSIZE    512

/* Plugin data structures */
static GkrellmMonitor *monitor;
static GtkWidget      *vbox;      /* Container for the chart */
static GkrellmChart   *chart;
static GkrellmChartconfig *chart_config;
static GkrellmChartdata *gpu_cd;
static GkrellmChartdata *vram_cd;
static GkrellmPanel   *panel;
static gint           style_id;

/* NVML handles */
static nvmlDevice_t device;
static gboolean nvml_initialized = FALSE;
static char gpu_name[NVML_DEVICE_NAME_BUFFER_SIZE];

/* Current values for display */
static gint current_gpu_percent = 0;
static gint current_vram_percent = 0;

/* Text format string - default similar to CPU */
static gchar *text_format = NULL;
static gboolean text_format_enable = TRUE;


/* Configuration variables */
static GtkWidget *text_format_combo_box;
static GtkWidget *text_format_entry;

/* Forward declarations */
static gint chart_expose_event(GtkWidget *widget, GdkEventExpose *ev);
static gint panel_expose_event(GtkWidget *widget, GdkEventExpose *ev);
static void cb_chart_click(GtkWidget *widget, GdkEventButton *ev);
static void format_gpu_text(void);
static void create_gpu_tab(GtkWidget *tab);
static void apply_gpu_config(void);

/* Initialize NVML */
static gboolean
init_nvml(void)
{
    nvmlReturn_t result;
    unsigned int device_count;
    
    if (nvml_initialized)
        return TRUE;
    
    result = nvmlInit();
    if (result != NVML_SUCCESS) {
        g_warning("Failed to initialize NVML: %s", nvmlErrorString(result));
        return FALSE;
    }
    
    result = nvmlDeviceGetCount(&device_count);
    if (result != NVML_SUCCESS || device_count == 0) {
        g_warning("No NVIDIA GPUs found");
        nvmlShutdown();
        return FALSE;
    }
    
    /* Use first GPU */
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        g_warning("Failed to get GPU handle: %s", nvmlErrorString(result));
        nvmlShutdown();
        return FALSE;
    }
    
    /* Get GPU name */
    result = nvmlDeviceGetName(device, gpu_name, NVML_DEVICE_NAME_BUFFER_SIZE);
    if (result != NVML_SUCCESS) {
        strcpy(gpu_name, "NVIDIA GPU");
    }
    
    nvml_initialized = TRUE;
    return TRUE;
}

/* Format text for GPU/VRAM display */
static void
format_gpu_text(void)
{
    gchar buf[512];
    gchar *s, *p, *format;
    gint len;
    
    if (!text_format || !chart)
        return;
    
    /* Parse the format string and build the complete text */
    format = g_strdup(text_format);
    p = buf;
    s = format;
    
    while (*s && (p - buf) < 500) {
        if (*s == '\\' && *(s+1)) {
            s++; /* Skip backslash */
            switch (*s) {
                case 'D':
                    /* Data selector - copy as-is for GKrellM to handle */
                    *p++ = '\\';
                    *p++ = 'D';
                    s++;
                    if (*s >= '0' && *s <= '1') {
                        *p++ = *s++;
                    }
                    break;
                case 'd':
                    /* Data selector (alternate form) */
                    *p++ = '\\';
                    *p++ = 'd';
                    s++;
                    if (*s >= '0' && *s <= '1') {
                        *p++ = *s++;
                    }
                    break;
                case 'f':
                    /* Format specifier */
                    *p++ = '\\';
                    *p++ = 'f';
                    s++;
                    break;
                case 'a':
                    /* Attribute */
                    *p++ = '\\';
                    *p++ = 'a';
                    s++;
                    if (*s) {
                        *p++ = *s++;
                    }
                    break;
                case 's':
                    /* Small font */
                    *p++ = '\\';
                    *p++ = 's';
                    s++;
                    break;
                case '.':
                    /* Literal dot */
                    *p++ = '.';
                    s++;
                    break;
                case 'n':
                    /* Newline */
                    *p++ = '\n';
                    s++;
                    break;
                case 'r':
                    /* Carriage return */
                    *p++ = '\r';
                    s++;
                    break;
                case 'w':
                    /* Width specifier */
                    *p++ = '\\';
                    *p++ = 'w';
                    s++;
                    /* Copy width digits */
                    while (*s && (*s >= '0' && *s <= '9')) {
                        *p++ = *s++;
                    }
                    break;
                default:
                    /* Unknown escape - copy as-is */
                    *p++ = '\\';
                    *p++ = *s++;
                    break;
            }
        }
        else if (*s == '$') {
            /* Variable substitution */
            s++;
            if (*s == 'g') {
                /* GPU percentage */
                len = g_snprintf(p, 500 - (p - buf), "%d", current_gpu_percent);
                p += len;
                s++;
            }
            else if (*s == 'v') {
                /* VRAM percentage */
                len = g_snprintf(p, 500 - (p - buf), "%d", current_vram_percent);
                p += len;
                s++;
            }
            else if (*s == 'G') {
                /* GPU with label */
                len = g_snprintf(p, 500 - (p - buf), "GPU %d%%", current_gpu_percent);
                p += len;
                s++;
            }
            else if (*s == 'V') {
                /* VRAM with label */
                len = g_snprintf(p, 500 - (p - buf), "VRAM %d%%", current_vram_percent);
                p += len;
                s++;
            }
            else {
                /* Unknown variable - copy as-is */
                *p++ = '$';
                if (*s) {
                    *p++ = *s++;
                }
            }
        }
        else {
            /* Regular character */
            *p++ = *s++;
        }
    }
    *p = '\0';
    
    g_free(format);
    
    /* Draw the formatted text - GKrellM will handle \D selectors */
    /* Use consistent text style for both data lines */
    gkrellm_chart_reuse_text_format(chart);
    
    /* Draw with explicit style to ensure consistent font size */
    gkrellm_draw_chart_text(chart, style_id, buf);
}

/* Update function - called periodically */
static void
update_gpu(void)
{
    nvmlUtilization_t utilization;
    nvmlMemory_t memory;
    nvmlReturn_t result;
    gint gpu_percent = 0;
    gint vram_percent = 0;
    GkrellmTicks *ticks;
    
    if (!nvml_initialized || !chart || !panel)
        return;
    
    /* Get GKrellM tick structure */
    ticks = gkrellm_ticks();
    
    /* Only update on second ticks to sync with other charts */
    if (!ticks->second_tick)
        return;
    
    /* Get GPU utilization */
    result = nvmlDeviceGetUtilizationRates(device, &utilization);
    if (result == NVML_SUCCESS) {
        gpu_percent = (gint)utilization.gpu;
    }
    
    /* Get memory info */
    result = nvmlDeviceGetMemoryInfo(device, &memory);
    if (result == NVML_SUCCESS) {
        if (memory.total > 0) {
            vram_percent = (gint)((memory.used * 100) / memory.total);
        }
    }
    
    /* Store current values for display */
    current_gpu_percent = gpu_percent;
    current_vram_percent = vram_percent;
    
    /* Store data in chart - values are already 0-100 */
    gkrellm_store_chartdata(chart, 0, gpu_percent, vram_percent);
    
    /* Draw chart and text */
    gkrellm_draw_chartdata(chart);
    
    /* Draw formatted text on chart if enabled */
    if (text_format_enable && text_format) {
        format_gpu_text();
    }
    
    gkrellm_draw_chart_to_screen(chart);
    
    /* Draw panel label */
    gkrellm_draw_panel_label(panel);
    gkrellm_draw_panel_layers(panel);
}

/* Create function */
static void
create_gpu(GtkWidget *parent_vbox, gint first_create)
{
    GkrellmStyle     *style;
    GdkPixmap        **data_pixmap;
    GdkPixmap        *grid_pixmap;
    
    /* Initialize NVML on first create */
    if (first_create) {
        if (!init_nvml()) {
            return;
        }
        
        /* Create container vbox */
        vbox = gtk_vbox_new(FALSE, 0);
        gtk_container_add(GTK_CONTAINER(parent_vbox), vbox);
        gtk_widget_show(vbox);
        
        /* Create chart and panel structures */
        chart = gkrellm_chart_new0();
        panel = gkrellm_panel_new0();
    }
    
    /* Get style information */
    style = gkrellm_meter_style(style_id);
    
    /* Create the chart - this will use loaded config if available */
    gkrellm_chart_create(vbox, monitor, chart, &chart_config);
    
    /* Get data in/out pixmaps for drawing lines */
    data_pixmap = gkrellm_data_in_pixmap();
    grid_pixmap = gkrellm_data_in_grid_pixmap();
    
    /* Add GPU utilization data - uses first data_in color (typically blue) */
    gpu_cd = gkrellm_add_chartdata(chart, &data_pixmap[0], grid_pixmap, _("GPU%"));
    gkrellm_set_chartdata_draw_style(gpu_cd, CHARTDATA_LINE);
    gkrellm_set_chartdata_flags(gpu_cd, CHARTDATA_ALLOW_HIDE);
    gkrellm_monotonic_chartdata(gpu_cd, FALSE);
    
    /* Get data out pixmaps for second line */
    data_pixmap = gkrellm_data_out_pixmap();
    grid_pixmap = gkrellm_data_out_grid_pixmap();
    
    /* Add VRAM usage data - uses data_out color (typically red/brown) */
    vram_cd = gkrellm_add_chartdata(chart, &data_pixmap[0], grid_pixmap, _("VRAM%"));
    gkrellm_set_chartdata_draw_style(vram_cd, CHARTDATA_LINE);
    gkrellm_set_chartdata_flags(vram_cd, CHARTDATA_ALLOW_HIDE);
    gkrellm_monotonic_chartdata(vram_cd, FALSE);
    
    /* Allocate chartdata for our two data sets */
    gkrellm_alloc_chartdata(chart);
    
    /* Configure chart scaling - only set defaults on first create */
    if (first_create) {
        /* Set fixed grid resolution for percentage data */
        gkrellm_set_chartconfig_auto_grid_resolution(chart_config, FALSE);
        gkrellm_set_chartconfig_grid_resolution(chart_config, 20); /* 20% per grid line */
        gkrellm_set_chartconfig_fixed_grids(chart_config, 5);  /* 5 grid lines = 0,20,40,60,80,100 */
    }
    
    /* Create minimal panel for GPU name */
    gkrellm_panel_configure(panel, gpu_name, style);
    gkrellm_panel_create(vbox, monitor, panel);
    
    /* Connect expose events on first create */
    if (first_create) {
        g_signal_connect(G_OBJECT(chart->drawing_area), "expose_event",
                        G_CALLBACK(chart_expose_event), NULL);
        g_signal_connect(G_OBJECT(panel->drawing_area), "expose_event",
                        G_CALLBACK(panel_expose_event), NULL);
        
        /* Connect click handler for chart configuration and text toggle */
        g_signal_connect(G_OBJECT(chart->drawing_area), "button_press_event",
                        G_CALLBACK(cb_chart_click), NULL);
    }
    
    /* Refresh chart if not first create */
    if (!first_create) {
        gkrellm_draw_chart_to_screen(chart);
    }
}

/* Chart click callback */
static void
cb_chart_click(GtkWidget *widget, GdkEventButton *ev)
{
    if (ev->button == 3 && chart_config) {
        /* Right-click - open configuration */
        gkrellm_chartconfig_window_create(chart);
    }
    else if (ev->button == 1) {
        /* Left-click - toggle text display */
        text_format_enable = !text_format_enable;
        /* Force a redraw */
        if (chart) {
            gkrellm_draw_chart_to_screen(chart);
        }
    }
}

/* Chart expose event handler */
static gint
chart_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
    gdk_draw_pixmap(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            chart->pixmap, ev->area.x, ev->area.y,
            ev->area.x, ev->area.y,
            ev->area.width, ev->area.height);
    return FALSE;
}

/* Panel expose event handler */
static gint
panel_expose_event(GtkWidget *widget, GdkEventExpose *ev)
{
    gdk_draw_pixmap(widget->window,
            widget->style->fg_gc[GTK_WIDGET_STATE(widget)],
            panel->pixmap, ev->area.x, ev->area.y,
            ev->area.x, ev->area.y,
            ev->area.width, ev->area.height);
    return FALSE;
}

/* Configuration */
static void
save_gpu_config(FILE *f)
{
    /* Save chart configuration */
    if (chart_config) {
        gkrellm_save_chartconfig(f, chart_config, "gpu", NULL);
    }
    
    /* Save text format settings */
    if (text_format) {
        fprintf(f, "gpu text_format %s\n", text_format);
    }
    fprintf(f, "gpu text_format_enable %d\n", text_format_enable);
}

static void
load_gpu_config(gchar *arg)
{
    gchar config[32], item[CFG_BUFSIZE];
    gint n;
    
    n = sscanf(arg, "%31s %[^\n]", config, item);
    if (n == 2) {
        if (strcmp(config, GKRELLM_CHARTCONFIG_KEYWORD) == 0) {
            /* Load chart configuration - we have 2 chartdata layers (GPU and VRAM) */
            gkrellm_load_chartconfig(&chart_config, item, 2);
        }
        else if (strcmp(config, "text_format") == 0) {
            if (text_format)
                g_free(text_format);
            text_format = g_strdup(item);
        }
        else if (strcmp(config, "text_format_enable") == 0) {
            sscanf(item, "%d", &text_format_enable);
        }
    }
}

/* Configuration tab */
static void
cb_text_format(GtkWidget *widget, gpointer data)
{
    gchar *s;
    GtkWidget *entry = (GtkWidget *)data;
    
    if (!entry)
        return;
        
    s = gkrellm_gtk_entry_get_text(&entry);
    if (text_format)
        g_free(text_format);
    text_format = g_strdup(s);
}

static void
cb_text_format_combo(GtkWidget *widget, gpointer data)
{
    gchar *s;
    GtkWidget *entry = (GtkWidget *)data;
    gint active;
    
    if (!entry)
        return;
        
    active = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    if (active >= 0) {
        s = gtk_combo_box_get_active_text(GTK_COMBO_BOX(widget));
        if (s) {
            gtk_entry_set_text(GTK_ENTRY(entry), s);
            g_free(s);
        }
    }
}

static void
create_gpu_tab(GtkWidget *tab_vbox)
{
    GtkWidget       *tabs;
    GtkWidget       *vbox, *vbox1;
    GtkWidget       *text;
    gint            i;
    static gchar    *text_format_presets[] = {
        "\\d0\\f$g%\\d1\\f$v%",
        "\\D0\\f\\ag\\.$g%\\D1\\f\\av\\.$v%", 
        "\\D0\\f$G\\D1\\f$V",
        "\\D0\\fGPU $g%\\D1\\fVRAM $v%",
        "\\ww\\D0\\f$g\\D1\\f$v",
        NULL
    };
    
    tabs = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tabs), GTK_POS_TOP);
    gtk_box_pack_start(GTK_BOX(tab_vbox), tabs, TRUE, TRUE, 0);
    
    /* Setup tab */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, _("Setup"));
    
    vbox1 = gkrellm_gtk_category_vbox(vbox, _("Format String for Chart Labels"),
                                      4, 0, TRUE);
    text_format_combo_box = gtk_combo_box_new_text();
    gtk_box_pack_start(GTK_BOX(vbox1), text_format_combo_box, FALSE, FALSE, 2);
    
    for (i = 0; text_format_presets[i] != NULL; ++i)
        gtk_combo_box_append_text(GTK_COMBO_BOX(text_format_combo_box),
                                  text_format_presets[i]);
    
    text_format_entry = gtk_entry_new();
    gtk_entry_set_max_length(GTK_ENTRY(text_format_entry), 256);
    gtk_box_pack_start(GTK_BOX(vbox1), text_format_entry, FALSE, FALSE, 2);
    gtk_entry_set_text(GTK_ENTRY(text_format_entry), text_format);
    g_signal_connect(G_OBJECT(text_format_entry), "changed",
                     G_CALLBACK(cb_text_format), text_format_entry);
    
    g_signal_connect(G_OBJECT(text_format_combo_box), "changed",
                     G_CALLBACK(cb_text_format_combo), text_format_entry);
    
    /* Info tab */
    vbox = gkrellm_gtk_framed_notebook_page(tabs, _("Info"));
    text = gkrellm_gtk_scrolled_text_view(vbox, NULL,
                GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gkrellm_gtk_text_view_append(text,
        _("GPU Monitor\n\n"
        "This plugin monitors NVIDIA GPU utilization and VRAM usage.\n\n"
        "Format String Codes:\n"
        "$g - GPU usage percentage\n"
        "$v - VRAM usage percentage\n"
        "$G - GPU usage with label\n"
        "$V - VRAM usage with label\n"
        "\\D0 - Use GPU color (blue)\n"
        "\\D1 - Use VRAM color (red)\n"
        "\\f - Format prefix\n"
        "\\a - Attribute prefix\n"
        "\\. - Literal dot\n"
        "\\n - New line\n"));
}

static void
apply_gpu_config(void)
{
    /* Configuration is applied immediately via callbacks */
}

/* Monitor structure */
static GkrellmMonitor plugin_mon = {
    CONFIG_NAME,        /* Name in config */
    0,                  /* Id (0 for plugin) */
    create_gpu,         /* Create function */
    update_gpu,         /* Update function */
    create_gpu_tab,     /* Config tab create */
    apply_gpu_config,   /* Config apply */
    save_gpu_config,    /* Config save */
    load_gpu_config,    /* Config load */
    "gpu",              /* Config keyword - must be lowercase */
    
    NULL,              /* Undefined 2 */
    NULL,              /* Undefined 1 */
    NULL,              /* Private data */
    
    MON_CPU,           /* Insert before CPU monitor */
    NULL,              /* Handle */
    NULL               /* Path */
};

/* Plugin initialization */
GkrellmMonitor *
gkrellm_init_plugin(void)
{
    /* Add custom style for theming */
    style_id = gkrellm_add_meter_style(&plugin_mon, STYLE_NAME);
    
    /* Initialize default text format if not set */
    if (!text_format) {
        text_format = g_strdup("\\D2$V\\D0\\t\\f$G");
    }
    
    monitor = &plugin_mon;
    return monitor;
}
