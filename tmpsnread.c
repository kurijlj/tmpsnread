/* tmpsnread - print temperature sensors data as csv.
 *
 *  Copyright (C) 2015 Ljubomir Kurij <kurijlj@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.

 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * Program reads temperature sensors data and prints read data as comma
 * separated values. Since it uses libsensors utilities, lm_sensors package is
 * mandatory.
 *
 * Idea behind this program, was to have tool that prints temperature sensors
 * data in format that can be easily fetched to other programs for displaying
 * (e.g. conky).
 *
 * 2015-03-07 Ljubomir Kurij <kurijlj@gmail.com>
 *
 * * tmpsnread.c: created.
 */


/**
 * Libraries required by framework.
 */
/* required by glib's function calls */
#include <glib.h>
/* required by _() and N_() macros */
#include <glib/gi18n.h>
/* required by g_strrstr */
#include <glib/gprintf.h>
/* required by gettext and others */
#include <libintl.h>
/* required by setlocale */
#include <locale.h>
/* required by: it is obvious */
#include <sensors/sensors.h>
/* required by EXIT_SUCCESS */
#include <stdlib.h>


#ifndef APP_NAME
#	define APP_NAME "tmpsnread"
#endif

#ifndef VERSION_STRING
#	define VERSION_STRING "1.0"
#endif

#ifndef YEAR_STRING
#	define YEAR_STRING "2015"
#endif

#ifndef AUTHOR_NAME
#	define AUTHOR_NAME "Ljubomir Kurij"
#endif

#ifndef AUTHOR_MAIL
#	define AUTHOR_MAIL "<kurijlj@gmail.com>"
#endif

#ifndef PATH_SEPARATOR
#	define PATH_SEPARATOR "/"
#endif


/**
 * Definitions required for localisation.
 */

#ifndef PACKAGE
#	define PACKAGE APP_NAME
#endif

#ifndef LOCALEDIR
#	define LOCALEDIR "/usr/share/locale/"
#endif


#define UNDEFMAXMIN -1


/**
 * GOption engine global variables declaration/definition.
 */
static GOptionContext *gcontext;
static gchar          *summary;
static gchar          *description;


/**
 * GOption engine command line options definition.
 */
static gboolean      version    = FALSE;
static gboolean      fahrenheit = FALSE;
static gchar        *delimiter;
static gchar        *snconfig;

static GOptionEntry  entries[]  =
{
	{
		"version",
		'V',
		0,
		G_OPTION_ARG_NONE,
		&version,
		N_("Print version information and then exit."),
		NULL
	},
	{
		"fahrenheit",
		'f',
		0,
		G_OPTION_ARG_NONE,
		&fahrenheit,
		N_("Show temperatures in degrees Fahrenheit."),
		NULL
	},
	{
		"delimiter",
		'd',
		0,
		G_OPTION_ARG_STRING,
		&delimiter,
		N_("Use DELIM instead of semicolon for field delimiter."),
		N_("DELIM")
	},
	{
		"config-file",
		'c',
		0,
		G_OPTION_ARG_FILENAME,
		&snconfig,
		N_("Specify a config file."),
		N_("CONFIG_FILE")
	},
	{ NULL }
};


/**
 * Name under which this program was invoked.
 */
const gchar *exec_name;


/**
 * init_goption:
 * @argc: command line arguments count
 * @argv: command line arguments vector
 * @gerr: buffer for storing error information
 *
 * Initialize GOption engine. If g_option_context_parse fails error information
 * is stored in gerror and FALSE is returned.
 *
 * Returns: TRUE if gcontext was parsed successfully, else it returns FALSE.
 */
static gboolean
init_goption (gint *argc, gchar ***argv, GError **gerr)
{
	summary = g_strdup_printf(_("print temperature sensors data as csv.\n"));
	description = g_strdup_printf(_("Report bugs to: %s\n"), AUTHOR_MAIL);

	gcontext = g_option_context_new (_("Program reads temperature sensors "
		"data and prints read data as\ncomma separated values."));
	g_option_context_add_main_entries (gcontext, entries, PACKAGE);
	g_option_context_set_summary (gcontext, summary);
	g_option_context_set_description (gcontext, description);

	if (!g_option_context_parse (gcontext, argc, argv, gerr))
	{
		return FALSE;
	}

	return TRUE;
}


/**
 * cleanup_goption:
 *
 * Cleanup memory allocated for GOption engine.
 */
static void
cleanup_goption (void)
{
	if ((GOptionContext *) NULL != gcontext)
		g_option_context_free (gcontext);
	if ((gchar *) NULL != summary)
		g_free (summary);
	if ((gchar *) NULL != description)
		g_free (description);
	if ((gchar *) NULL != delimiter)
		g_free (delimiter);
	if ((gchar *) NULL != snconfig)
		g_free (snconfig);
}


/**
 * print_version:
 *
 * Print version information for application.
 */
static void
print_version (void)
{
	g_print ("%s%s%s%s%s%s%s%s", APP_NAME, " ", VERSION_STRING, " \
Copyright (C) ", YEAR_STRING, " ", AUTHOR_NAME, _(".\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\n"));
}


/**
 * try_help:
 *
 * Print try_help message on error.
 */
inline static void
try_help (void)
{
	g_printerr (_("Try '%s --help' for more information.\n\n"), exec_name);
}


/**
 * reset_gerror:
 * @gerr: memory to be released
 *
 * Release memory allocated for GError structure and set pointer to NULL.
 * Handles NULL pointer gracefully.
 */
static void
reset_gerror (GError **gerr)
{
	if ((GError *) NULL != *gerr)
	{
		g_error_free (*gerr);
		*gerr = (GError *) NULL;
	}
}


typedef struct {
	gchar   *chipname;
	gchar   *featurename;
	gdouble  input;
	gdouble  nrm_in;
	gdouble  min;
	gdouble  max;
	gdouble  critical;
} snreading;


/**
 * celsius_to_fahrenheit:
 * @c: value of temperature in degrees Celsius
 *
 * Convert value of temperature in degrees Celsius to degrees Fahrenheit.
 *
 * Returns: value of temperature in degrees Fahrenheit.
 */
inline static gdouble
celsius_to_fahrenheit (gdouble c)
{
	gdouble f;

	f = (c * (9.0/5.0)) + 32.0;

	return f;
}


/**
 * normalize_input:
 * @input: parameter that takes values in range [@min, @max]
 * @min: lower limit of range of values
 * @max: upper limit of range of values
 *
 * Normalize input values to 100. Where 0 corresponds to @min and 100
 * corresponds to @max.
 *
 * Returns: normalized value of @input. If @max is not available, or @input is
 * not available, or both it returns UNDEFMAXMIN.
 */
static gdouble
normalize_input (gdouble input, gdouble min, gdouble max)
{
	gdouble prcnt;

	/**
	 * If minimum reading is not available set it to 0.0.
	 * If maximum reading is not available return UNDEFMAXMIN.
	 * If input reading is not available return UNDEFMAXMIN.
	 */
	if (UNDEFMAXMIN == min) min = 0.0;
	if (UNDEFMAXMIN == max) return UNDEFMAXMIN;
	if (UNDEFMAXMIN == input) return UNDEFMAXMIN;

	prcnt = (input / (max - min)) * 100.0;

	return prcnt;
}


/**
 * read_sensor:
 * @name: chip name for which to read data
 * @feat: which chip feature to read
 * @read: read data
 *
 * For given chip name and given feature read data from ship sensors and store
 * it in a snreading structure. Since we only read one feature from chip
 * (SENSORS_FEATURE_TEMP) parameter @feat is redundant. Perhaps it will be
 * removed in following releases.
 *
 */
static void
read_sensor (const sensors_chip_name *name,
	const sensors_feature *feat,
	snreading * const read)
{
	const sensors_subfeature *subf;
	double input = UNDEFMAXMIN;
	double min   = UNDEFMAXMIN;
	double max   = UNDEFMAXMIN;
	double crit  = UNDEFMAXMIN;

	if ((subf = sensors_get_subfeature (name,
		feat,
		SENSORS_SUBFEATURE_TEMP_INPUT)))
	{
		sensors_get_value (name, subf->number, &input);
	}

	if ((subf = sensors_get_subfeature (name,
		feat,
		SENSORS_SUBFEATURE_TEMP_MIN)))
	{
		sensors_get_value (name, subf->number, &min);
	}

	if ((subf = sensors_get_subfeature (name,
		feat,
		SENSORS_SUBFEATURE_TEMP_MAX)))
	{
		sensors_get_value (name, subf->number, &max);
	}

	if ((subf = sensors_get_subfeature (name,
		feat,
		SENSORS_SUBFEATURE_TEMP_CRIT)))
	{
		sensors_get_value (name, subf->number, &crit);
	}

	read->input    = input;
	read->nrm_in   = normalize_input (input, min, max);
	read->min      = min;
	read->max      = max;
	read->critical = crit;
}


/**
 * print_sensor_reading:
 * @data: data read from chip to be formated and printed to stdout
 *
 * Format and print data read from chip to stdout.
 *
 */
static void
print_sensor_reading (const snreading *data)
{
	gchar *ins, *nrm_ins, *mins, *mxs, *crts;

	if (UNDEFMAXMIN == data->input)
		ins = g_strdup_printf ("N/A");
	else
		ins = g_strdup_printf ("%5.2f",
			fahrenheit ? celsius_to_fahrenheit (data->input)
			: data->input);

	if (UNDEFMAXMIN == data->nrm_in)
		nrm_ins = g_strdup_printf ("N/A");
	else
		nrm_ins = g_strdup_printf ("%5.2f", data->nrm_in);

	if (UNDEFMAXMIN == data->min)
		mins = g_strdup_printf ("N/A");
	else
		mins = g_strdup_printf ("%5.2f",
			fahrenheit ? celsius_to_fahrenheit (data->min)
			: data->min);

	if (UNDEFMAXMIN == data->max)
		mxs = g_strdup_printf ("N/A");
	else
		mxs = g_strdup_printf ("%5.2f",
			fahrenheit ? celsius_to_fahrenheit (data->max)
			: data->max);

	if (UNDEFMAXMIN == data->critical)
		crts = g_strdup_printf ("N/A");
	else
		crts = g_strdup_printf ("%5.2f",
			fahrenheit ? celsius_to_fahrenheit (data->critical)
			: data->critical);

	g_print ("%s%c%s%c%c%c%s%c%s%c%s%c%s%c%s\n",
		data->chipname,
		delimiter[0],
		data->featurename,
		delimiter[0],
		fahrenheit ? 'F' : 'C',
		delimiter[0],
		ins,
		delimiter[0],
		nrm_ins,
		delimiter[0],
		mins,
		delimiter[0],
		mxs,
		delimiter[0],
		crts);

	g_free (ins);
	g_free (mins);
	g_free (mxs);
	g_free (crts);
}


/**
 * print_sensors:
 *
 * Gather and print temperature data from various system chips.
 *
 */
void
print_sensors (void)
{
	gint chipnum = 0;
	gint cnt     = 0;
	const sensors_chip_name *name = (sensors_chip_name *) NULL;

	g_print ("chip name%cfeature name%cunits%cinput%cnormalized input [%%]"
		"%cmin%cmax%ccritical\n",
		delimiter[0],
		delimiter[0],
		delimiter[0],
		delimiter[0],
		delimiter[0],
		delimiter[0],
		delimiter[0]);

	while (NULL != (name = sensors_get_detected_chips (NULL, &chipnum)))
	{
		int i = 0;
		const sensors_feature *feature;

		while (NULL != (feature = sensors_get_features (name, &i)))
		{
			if (SENSORS_FEATURE_TEMP == feature->type)
			{
				snreading read;

				read_sensor (name, feature, &read);

				read.chipname    = name->prefix;
				read.featurename = feature->name;

				print_sensor_reading (&read);
			}
		}

		cnt++;
	}

	if (0 == cnt)
	{
		g_print ("N/A%cN/A%cN/A%cN/A%cN/A%cN/A%cN/A%cN/A\n",
			delimiter[0],
			delimiter[0],
			delimiter[0],
			delimiter[0],
			delimiter[0],
			delimiter[0],
			delimiter[0]);

		g_printerr (
			_("%s: No sensors found!\n"
			  "%s: Make sure you loaded "
			  "all the kernel drivers you need.\n"
			  "%s: Try sensors-detect to find "
			  "out which these are.\n"),
			exec_name,
			exec_name,
			exec_name);
	}

	g_print ("\n");
}


/**
 * main:
 *
 * Everything begins here.
 *
 */
int
main (int argc, char *argv[])
{
	GError *gerror = (GError *) NULL;

	/**
	 * Construct the name of the executable, without the directory part.
	 * The strrchr() function returns a pointer to the last occurrence of
	 * the character in the string.
	 */
	exec_name = g_strrstr (argv[0], PATH_SEPARATOR);
	if (!exec_name)
		exec_name = argv[0];
	else
		++exec_name;

	/**
	 * Enable native language support.
	 */
	setlocale (LC_ALL, "");
	bindtextdomain (PACKAGE, LOCALEDIR);
	bind_textdomain_codeset(PACKAGE, "utf-8");
	textdomain (PACKAGE);

	/**
	 * Initialize GOption engine.
	 */
	if (!init_goption (&argc, &argv, &gerror))
	{
		g_printerr (_("%s: cannot initialize : %s\n\n"),
			exec_name,
			gerror->message);
		reset_gerror (&gerror);
		exit (EXIT_FAILURE);
	}

	/**
	 * If user typed in version parameter display version information and
	 * bail out.
	 */
	if (TRUE == version)
	{
		print_version ();
		cleanup_goption ();
		exit (EXIT_SUCCESS);
	}

	/**
	 * If user run application without any parameter, print help and exit
	 * with error.
	 */
	if (FALSE) /* Test for switches here */
	{
		gchar *ht;

		ht = g_option_context_get_help (gcontext, TRUE, NULL);
		g_print ("%s", ht);

		g_free (ht);
		cleanup_goption ();
		exit (EXIT_SUCCESS);
	}

	/**
	 * From this point we can do the rest of initialization process.
	 */

	FILE *snconfigfile = (FILE *) NULL;

	/* Open the config file if specified. */
	if (snconfig && NULL != (snconfigfile = fopen (snconfig, "r")))
	{
		g_printerr (_("%s: cannot open file : %s\n\n"),
			exec_name,
			snconfig);
		cleanup_goption ();
		exit (EXIT_FAILURE);
	}

	/* Initialize the sensors library. */
	if (0 != sensors_init (snconfigfile))
	{
		g_printerr (_("%s: cannot initialize : libsensors\n\n"),
			exec_name);
		cleanup_goption ();
		exit (EXIT_FAILURE);
	}

	if (delimiter)
	{
		if (delimiter[0] != '\0' && delimiter[1] != '\0')
		{
			g_printerr (_("%s: the delimiter must be a single"
				"character\n"),
				exec_name);
			try_help ();
			sensors_cleanup ();
			cleanup_goption ();
			exit (EXIT_FAILURE);
		}
	}
	else
		delimiter = g_strdup (";");


	print_sensors ();


	/**
	 * Do the cleanup and end program.
	 */
	sensors_cleanup ();
	cleanup_goption ();

	return EXIT_SUCCESS;
}
