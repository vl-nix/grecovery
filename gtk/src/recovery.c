/*
* Copyright 2021 Stepan Perun
*
* This program is free software.
*
* License: Gnu General Public License GPL-2
* file:///usr/share/common-licenses/GPL-2
* http://www.gnu.org/licenses/gpl-2.0.html
*/

#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <ctype.h> 
#include <locale.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <config.h>
#include <common.h>
#include <hdcache.h>
#include <hdaccess.h>
#include <fnctdsk.h>
#include <filegen.h>
#include <sessionp.h>
#include <list.h>
#include <intrf.h>
#include <phcfg.h>
#include <partauto.h>
#include <photorec.h>
#include <log.h>
#include <log_part.h>

#include "recovery.h"
#include "formats-win.h"

#include "../subprojects/lib/src/dir.h"
#include "../subprojects/lib/src/fat.h"
#include "../subprojects/lib/src/fat_dir.h"
#include "../subprojects/lib/src/file_tar.h"
#include "../subprojects/lib/src/pnext.h"
#include "../subprojects/lib/src/file_found.h"
#include "../subprojects/lib/src/psearch.h"
#include "../subprojects/lib/src/photorec_check_header.h"

#define READ_SIZE 1024*512

extern const arch_fnct_t arch_none;
extern file_enable_t array_file_enable[];

extern const file_hint_t file_hint_tar;
extern file_check_list_t file_check_list;

struct _Recovery
{
	GObject parent_instance;

	disk_t      	*selt_disk;
	partition_t 	*selt_part;

	list_disk_t		*list_disk;
	list_part_t 	*list_part;

	struct ph_param 	*params;
	struct ph_options 	*options;

	gboolean search_only;
	gboolean recovery_done;
	gboolean stop_recovery;
};

G_DEFINE_TYPE ( Recovery, recovery, G_TYPE_OBJECT )

static char * recovery_file_open ( const char *msg, const char *path, const char *accept, const char *icon, uint8_t num )
{
	GtkFileChooserDialog *dialog = ( GtkFileChooserDialog *)gtk_file_chooser_dialog_new (
		msg, NULL, num, "gtk-cancel", GTK_RESPONSE_CANCEL, accept, GTK_RESPONSE_ACCEPT, NULL );

	gtk_window_set_icon_name ( GTK_WINDOW ( dialog ), icon );
	gtk_file_chooser_set_current_folder ( GTK_FILE_CHOOSER ( dialog ), path );
	gtk_file_chooser_set_select_multiple ( GTK_FILE_CHOOSER ( dialog ), FALSE );

	char *filename = NULL;

	if ( gtk_dialog_run ( GTK_DIALOG ( dialog ) ) == GTK_RESPONSE_ACCEPT )
		filename = gtk_file_chooser_get_filename ( GTK_FILE_CHOOSER ( dialog ) );

	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );

	return filename;
}

static void recovery_message_dialog ( const char *f_error, const char *file_or_info, GtkMessageType mesg_type, GtkWindow *window )
{
	GtkMessageDialog *dialog = ( GtkMessageDialog *)gtk_message_dialog_new (
					window,    GTK_DIALOG_MODAL,
					mesg_type, GTK_BUTTONS_CLOSE,
					"%s\n%s",  f_error, file_or_info );

	gtk_dialog_run     ( GTK_DIALOG ( dialog ) );
	gtk_widget_destroy ( GTK_WIDGET ( dialog ) );
}

static void recovery_no_disk_warn ( GtkWindow *window, Recovery *recovery )
{
	recovery_message_dialog ( "No harddisk found.", "Root? ... Add disk?", GTK_MESSAGE_WARNING, window );
}

static pstatus_t photorec_find_blocksize ( alloc_data_t *list_search_space, Recovery *recovery )
{
	uint64_t offset = 0;
	unsigned char *buffer_start;
	unsigned char *buffer_olddata;
	unsigned char *buffer;
	unsigned int buffer_size;
	const unsigned int blocksize = recovery->params->blocksize;
	const unsigned int read_size = ( blocksize > 65536 ? blocksize : 65536 );

	time_t start_time;
	time_t previous_time;

	alloc_data_t *current_search_space;
	file_recovery_t file_recovery;

	recovery->params->file_nbr = 0;
	reset_file_recovery ( &file_recovery );
	file_recovery.blocksize = blocksize;

	buffer_size = blocksize + READ_SIZE;
	buffer_start = (unsigned char *)MALLOC(buffer_size);
	buffer_olddata = buffer_start;
	buffer = buffer_olddata + blocksize;

	start_time = time ( NULL );

	previous_time = start_time;
	memset ( buffer_olddata, 0, blocksize );
	current_search_space = td_list_entry ( list_search_space->list.next, alloc_data_t, list );

	if ( current_search_space != list_search_space )
		offset = current_search_space->start;

	if ( recovery->options->verbose > 0 )
		info_list_search_space ( list_search_space, current_search_space, recovery->params->disk->sector_size, 0, recovery->options->verbose );

	while ( gtk_events_pending () ) gtk_main_iteration ();

	recovery->params->offset = offset;
	recovery->params->disk->pread ( recovery->params->disk, buffer, READ_SIZE, offset );

	while ( current_search_space != list_search_space )
	{
		uint64_t old_offset = offset;

		{
			file_recovery_t file_recovery_new;

			if ( file_recovery.file_stat != NULL && file_recovery.file_stat->file_hint == &file_hint_tar &&
	  			is_valid_tar_header ( (const struct tar_posix_header *) (buffer-0x200 ) ) )
			{
				/* Currently saving a tar, do not check the data for know header */
			}
			else
			{
				const struct td_list_head *tmpl;
				file_recovery_new.file_stat = NULL;

				td_list_for_each ( tmpl, &file_check_list.list )
				{
					const struct td_list_head *tmp;
					const file_check_list_t *tmp2 = td_list_entry_const ( tmpl, const file_check_list_t, list );

					td_list_for_each ( tmp, &tmp2->file_checks[buffer[tmp2->offset]].list )
					{
						const file_check_t *file_check = td_list_entry_const ( tmp, const file_check_t, list );

						if ( ( file_check->length == 0 || memcmp ( buffer + file_check->offset, file_check->value, file_check->length ) == 0 ) &&
							file_check->header_check ( buffer, read_size, 1, &file_recovery, &file_recovery_new ) != 0 )
						{
							file_recovery_new.file_stat = file_check->file_stat; break;
						}
					}

					if ( file_recovery_new.file_stat != NULL ) break;
				}

        		if ( file_recovery_new.file_stat != NULL && file_recovery_new.file_stat->file_hint != NULL )
				{
					/* A new file begins, backup file offset */
					current_search_space = file_found ( current_search_space, offset, file_recovery_new.file_stat );
					recovery->params->file_nbr++;
					file_recovery_cpy ( &file_recovery, &file_recovery_new );
				}
			}
		}

		if ( file_recovery.file_stat != NULL ) /* Check for data EOF */
		{
			data_check_t res = DC_CONTINUE;

			if ( file_recovery.data_check != NULL )
				res = file_recovery.data_check ( buffer_olddata, 2 * blocksize, &file_recovery );

			file_recovery.file_size += blocksize;

			if ( res == DC_STOP || res == DC_ERROR ) /* EOF found */
				reset_file_recovery ( &file_recovery );
		}

		/* Check for maximum filesize */
		if ( file_recovery.file_stat != NULL && file_recovery.file_stat->file_hint->max_filesize > 0 && file_recovery.file_size >= file_recovery.file_stat->file_hint->max_filesize )
			reset_file_recovery ( &file_recovery );

		if ( recovery->params->file_nbr >= 10 )
			current_search_space = list_search_space;
		else
			get_next_sector ( list_search_space, &current_search_space, &offset, blocksize );

		if ( current_search_space == list_search_space ) /* End of disk found => EOF */
			reset_file_recovery ( &file_recovery );
		else
			recovery->params->offset = offset;

		buffer_olddata += blocksize;
		buffer += blocksize;

		if ( old_offset + blocksize != offset || buffer + read_size > buffer_start + buffer_size )
		{
			memcpy ( buffer_start, buffer_olddata, blocksize );
			buffer_olddata = buffer_start;
			buffer = buffer_olddata + blocksize;

			if ( recovery->options->verbose > 1 )
			{
				log_verbose ( "Reading sector %10llu/%llu\n",
					(unsigned long long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size ),
					(unsigned long long)( (recovery->params->partition->part_size-1) / recovery->params->disk->sector_size) );
			}

			if ( recovery->params->disk->pread ( recovery->params->disk, buffer, READ_SIZE, offset ) != READ_SIZE )
			{
#ifdef HAVE_NCURSES_XXX
				wmove(stdscr,11,0);
				wclrtoeol(stdscr);
				wprintw(stdscr,"Error reading sector %10lu\n",
					(unsigned long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size) );
#endif
			}	

			{
				time_t current_time;
				current_time = time(NULL);

				if ( current_time > previous_time )
				{
					previous_time=current_time;

					while ( gtk_events_pending () ) gtk_main_iteration ();

					if ( recovery->stop_recovery )
					{
						log_info("PhotoRec has been stopped\n");
						current_search_space = list_search_space;
					}
				}
			}
		}

	}

	free ( buffer_start );

	return PSTATUS_OK;
}

static pstatus_t photorec_aux ( alloc_data_t *list_search_space, Recovery *recovery )
{
	uint64_t offset;
	unsigned char *buffer_start;
	unsigned char *buffer_olddata;
	unsigned char *buffer;
	unsigned int buffer_size;
	const unsigned int blocksize = recovery->params->blocksize; 
	const unsigned int read_size = ( blocksize > 65536 ? blocksize : 65536 );
	uint64_t offset_before_back = 0;
	unsigned int back = 0;

	time_t start_time;
	time_t previous_time;
	time_t next_checkpoint;
	pstatus_t ind_stop = PSTATUS_OK;

	pfstatus_t file_recovered_old = PFSTATUS_BAD;
	alloc_data_t *current_search_space;
	file_recovery_t file_recovery;
	memset ( &file_recovery, 0, sizeof(file_recovery) );
	reset_file_recovery ( &file_recovery );
	file_recovery.blocksize = blocksize;

	buffer_size = blocksize + READ_SIZE;
	buffer_start = (unsigned char *)MALLOC(buffer_size);
	buffer_olddata = buffer_start;
	buffer = buffer_olddata + blocksize;

	start_time = time(NULL);
	previous_time = start_time;
	next_checkpoint = ( start_time + 5 ) * 60;

	memset ( buffer_olddata, 0, blocksize );
	current_search_space = td_list_first_entry ( &list_search_space->list, alloc_data_t, list );
	offset = set_search_start ( recovery->params, &current_search_space, list_search_space );

	if ( recovery->options->verbose > 0 )
		info_list_search_space ( list_search_space, current_search_space, recovery->params->disk->sector_size, 0, recovery->options->verbose );

	if ( recovery->options->verbose > 1 )
	{
		log_verbose ( "Reading sector %10llu/%llu\n",
			(unsigned long long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size ),
			(unsigned long long)( (recovery->params->partition->part_size-1) / recovery->params->disk->sector_size) );
	}

	recovery->params->disk->pread ( recovery->params->disk, buffer, READ_SIZE, offset );
	header_ignored(NULL);

	while ( current_search_space != list_search_space )
	{
		uint64_t old_offset = offset;
		pfstatus_t file_recovered = PFSTATUS_BAD;
		data_check_t data_check_status = DC_SCAN;

#ifdef DEBUG
		log_debug ( "sector %llu\n",
			(unsigned long long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size) );

		if ( !(current_search_space->start <= offset && offset <= current_search_space->end) )
		{
			log_critical ( "BUG: offset=%llu not in [%llu-%llu]\n",
				(unsigned long long)(offset / recovery->params->disk->sector_size),
				(unsigned long long)(current_search_space->start / recovery->params->disk->sector_size),
				(unsigned long long)(current_search_space->end / recovery->params->disk->sector_size) );

			log_close();
			exit(1);
		}
#endif

		ind_stop = photorec_check_header ( &file_recovery, recovery->params, recovery->options, list_search_space, buffer, &file_recovered, offset );

		if ( file_recovery.file_stat != NULL )
		{
			/* try to skip ext2/ext3 indirect block */
			if ( (recovery->params->status == STATUS_EXT2_ON || recovery->params->status == STATUS_EXT2_ON_SAVE_EVERYTHING ) &&
				file_recovery.file_size >= 12*blocksize && ind_block ( buffer, blocksize ) != 0 )
			{
				file_block_append ( &file_recovery, list_search_space, &current_search_space, &offset, blocksize, 0 );
				data_check_status = DC_CONTINUE;

				if ( recovery->options->verbose > 1 )
				{
					log_verbose ( "Skipping sector %10lu/%lu\n",
						(unsigned long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size),
						(unsigned long)( (recovery->params->partition->part_size-1) / recovery->params->disk->sector_size) );
				}

				memcpy ( buffer, buffer_olddata, blocksize );
			}
			else
			{
				if ( file_recovery.handle != NULL )
				{
					if ( fwrite ( buffer, blocksize, 1, file_recovery.handle ) < 1 )
					{
						log_critical ( "Cannot write to file %s after %llu bytes: %s\n", 
							file_recovery.filename, (long long unsigned)file_recovery.file_size, strerror(errno) );

						if( errno == EFBIG ) /* File is too big for the destination filesystem */
						{
							data_check_status = DC_STOP;
						}
						else /* Warn the user */
						{
							ind_stop = PSTATUS_ENOSPC;
							recovery->params->offset = file_recovery.location.start;
						}
					}
				}

				if ( ind_stop == PSTATUS_OK )
				{
					file_block_append ( &file_recovery, list_search_space, &current_search_space, &offset, blocksize, 1 );

					if ( file_recovery.data_check != NULL )
						data_check_status = file_recovery.data_check ( buffer_olddata, 2 * blocksize, &file_recovery );
					else
						data_check_status = DC_CONTINUE;

					file_recovery.file_size += blocksize;

					if ( data_check_status == DC_STOP )
					{
						if ( recovery->options->verbose > 1 ) log_trace ( "EOF found\n" );

						g_signal_emit_by_name ( recovery, "recovery-set-file", recovery->params->file_nbr, file_recovery.filename, file_recovery.file_size, file_recovery.location.start );
					}
				}
			}

			if ( data_check_status != DC_STOP && data_check_status != DC_ERROR && file_recovery.file_stat->file_hint->max_filesize > 0 && file_recovery.file_size >= file_recovery.file_stat->file_hint->max_filesize )
			{
				data_check_status = DC_STOP;

				log_verbose ( "File should not be bigger than %llu, stop adding data\n",
					(long long unsigned)file_recovery.file_stat->file_hint->max_filesize );
			}

			if ( data_check_status != DC_STOP && data_check_status != DC_ERROR && file_recovery.file_size + blocksize >= PHOTOREC_MAX_SIZE_32 && is_fat(recovery->params->partition) )
			{
				data_check_status = DC_STOP;

				log_verbose ( "File should not be bigger than %llu, stop adding data\n",
					(long long unsigned)file_recovery.file_stat->file_hint->max_filesize );
			}

			if ( data_check_status == DC_STOP || data_check_status == DC_ERROR )
			{
				if ( data_check_status == DC_ERROR )
					file_recovery.file_size = 0;

				file_recovered = file_finish2 ( &file_recovery, recovery->params, recovery->options->paranoid, list_search_space );

				if ( recovery->options->lowmem > 0 ) forget ( list_search_space,current_search_space );
			}
		}

		if ( ind_stop != PSTATUS_OK )
		{
			log_info ( "PhotoRec has been stopped\n" );
			file_recovery_aborted ( &file_recovery, recovery->params, list_search_space );
			free ( buffer_start );
			return ind_stop;
		}

		if ( file_recovered == PFSTATUS_BAD )
		{
			if ( data_check_status == DC_SCAN )
			{
				if ( file_recovered_old == PFSTATUS_OK )
				{
					offset_before_back=offset;

					if ( back < 5 && get_prev_file_header ( list_search_space, &current_search_space, &offset ) == 0 )
					{
						back++;
					}
					else
					{
						back = 0;
						get_prev_location_smart ( list_search_space, &current_search_space, &offset, file_recovery.location.start );
					}
				}
				else
				{
					get_next_sector ( list_search_space, &current_search_space, &offset, blocksize );

					if ( offset > offset_before_back ) back = 0;
				}
			}
		}
		else if ( file_recovered == PFSTATUS_OK_TRUNCATED ) /* try to recover the previous file, otherwise stay at the current location */
		{
			offset_before_back = offset;

			if ( back < 5 && get_prev_file_header ( list_search_space, &current_search_space, &offset) == 0 )
			{
				back++;
			}
			else
			{
				back = 0;
				get_prev_location_smart ( list_search_space, &current_search_space, &offset, file_recovery.location.start );
			}
		}

		if ( current_search_space == list_search_space )
		{
#ifdef DEBUG_GET_NEXT_SECTOR
			log_trace ( "current_search_space==list_search_space=%p (prev=%p,next=%p)\n",
				current_search_space, current_search_space->list.prev, current_search_space->list.next );

			log_trace ( "End of media\n" );
#endif

			file_recovered = file_finish2 ( &file_recovery, recovery->params, recovery->options->paranoid, list_search_space );

			if ( file_recovered != PFSTATUS_BAD )
				get_prev_location_smart ( list_search_space, &current_search_space, &offset, file_recovery.location.start );

			if ( recovery->options->lowmem > 0 )
				forget ( list_search_space, current_search_space );
		}

		buffer_olddata += blocksize;
		buffer += blocksize;

		if ( file_recovered != PFSTATUS_BAD || old_offset + blocksize != offset || buffer + read_size > buffer_start + buffer_size )
		{
			if ( file_recovered != PFSTATUS_BAD )
				memset ( buffer_start, 0, blocksize );
			else
				memcpy ( buffer_start, buffer_olddata, blocksize );

			buffer_olddata = buffer_start;
			buffer = buffer_olddata + blocksize;

			if ( recovery->options->verbose > 1 )
			{
				log_verbose ( "Reading sector %10llu/%llu\n",
					(unsigned long long)( (offset - recovery->params->partition->part_offset) / recovery->params->disk->sector_size ),
					(unsigned long long)( (recovery->params->partition->part_size-1) / recovery->params->disk->sector_size) );
			}

			if ( recovery->params->disk->pread ( recovery->params->disk, buffer, READ_SIZE, offset ) != READ_SIZE )
			{
			}

			if ( ind_stop == PSTATUS_OK )
			{
				const time_t current_time = time(NULL);

				if ( current_time > previous_time )
				{
					recovery->params->offset = offset;
					previous_time = current_time;

					while ( gtk_events_pending () ) gtk_main_iteration ();

					if ( recovery->stop_recovery )
					{
						log_info ( "GRecovery has been stopped\n" );

						file_recovery_aborted ( &file_recovery, recovery->params, list_search_space );
						free(buffer_start);

						return PSTATUS_STOP;
					}

					if ( current_time >= next_checkpoint )
						next_checkpoint = regular_session_save ( list_search_space, recovery->params, recovery->options, current_time );
				}
			}
		}

		file_recovered_old = file_recovered;
	}

	free ( buffer_start );

	return ind_stop;
}

static int photorec ( alloc_data_t *list_search_space, Recovery *recovery )
{
	pstatus_t ind_stop = PSTATUS_OK;

	const unsigned int blocksize_is_known = recovery->params->blocksize;

	params_reset ( recovery->params, recovery->options );
	recovery->params->dir_num = photorec_mkdir ( recovery->params->recup_dir, recovery->params->dir_num );

	for ( recovery->params->pass=0; recovery->params->status != STATUS_QUIT; recovery->params->pass++ )
	{
		switch ( recovery->params->status )
		{
			case STATUS_UNFORMAT:
				break;

			case STATUS_FIND_OFFSET:
			{
				uint64_t start_offset = 0;

				if ( blocksize_is_known > 0 )
				{
					ind_stop = PSTATUS_OK;

					if ( !td_list_empty ( &list_search_space->list ) )
						start_offset = ( td_list_entry ( list_search_space->list.next, alloc_data_t, list ) )->start % recovery->params->blocksize;
				}
				else
				{
					ind_stop = photorec_find_blocksize ( list_search_space, recovery );
					recovery->params->blocksize = find_blocksize ( list_search_space, recovery->params->disk->sector_size, &start_offset );
				}

				update_blocksize ( recovery->params->blocksize, list_search_space, start_offset );
			}
				break;
  
			case STATUS_EXT2_ON_BF:
			case STATUS_EXT2_OFF_BF:
				break;

			default:
				ind_stop = photorec_aux ( list_search_space, recovery );
				break;
		}

		session_save ( list_search_space, recovery->params, recovery->options );

		switch ( ind_stop )
		{
			case PSTATUS_EACCES:
			case PSTATUS_ENOSPC:
			{
				g_autofree char *directory = recovery_file_open ( "Not enough space! Select Folder", 
					recovery->params->recup_dir, "gtk-open", "document-open", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER );

				if ( !directory )
				{
					recovery->params->status = STATUS_QUIT;
				}
				else
				{
					free ( recovery->params->recup_dir );
					recovery->params->recup_dir = g_strconcat ( directory, "/", DEFAULT_RECUP_DIR, NULL );
					recovery->params->dir_num = photorec_mkdir ( recovery->params->recup_dir, recovery->params->dir_num );
				}
			}
				break;

			case PSTATUS_OK:
				status_inc ( recovery->params, recovery->options );
				if ( recovery->params->status == STATUS_QUIT ) unlink ( "photorec.ses" );
				break;

			case PSTATUS_STOP:
				recovery->params->status = STATUS_QUIT;
				break;
		}

		update_stats ( recovery->params->file_stats, list_search_space );
	}

	free_search_space ( list_search_space );
	free_header_check ();

	free ( recovery->params->file_stats );
	recovery->params->file_stats = NULL;

	return 0;
}

void recovery_search_update ( GtkLabel *prg_label, GtkProgressBar *prg_bar, Recovery *recovery )
{
	if ( recovery->selt_disk == NULL || recovery->selt_part == NULL ) return;

	const partition_t *partition = recovery->params->partition;
	const unsigned int sector_size = recovery->params->disk->sector_size;

	char tmp[128], tmp_a[256];

	if ( recovery->params->status == STATUS_QUIT )
	{
		sprintf ( tmp, "%s", "Recovery completed" );
	}
	else if ( recovery->params->status == STATUS_EXT2_ON_BF || recovery->params->status == STATUS_EXT2_OFF_BF )
	{
		const unsigned long sectors_remaining = ( recovery->params->offset - partition->part_offset ) / sector_size;
		sprintf ( tmp, "Bruteforce %lu sectors remaining (test %u)", sectors_remaining, recovery->params->pass );
	}
	else
	{
		const unsigned long long sector_current = ( recovery->params->offset>partition->part_offset && recovery->params->offset < partition->part_size ?
			( ( recovery->params->offset-partition->part_offset ) / sector_size ) : 0 );

		const unsigned long long sector_total = partition->part_size / sector_size;
		sprintf ( tmp, "Pass %u - Sector %llu / %llu", recovery->params->pass, sector_current, sector_total );
	}

	if ( recovery->params->status==STATUS_FIND_OFFSET )
		sprintf ( tmp_a, "%s   %u/10 headers found", tmp, recovery->params->file_nbr );
	else
		sprintf ( tmp_a, "%s   %u files found", tmp, recovery->params->file_nbr );

	gtk_label_set_text ( prg_label, tmp_a );

	if ( recovery->params->status == STATUS_QUIT )
	{
		gtk_progress_bar_set_fraction ( prg_bar, 1.0 );
	}
	else if ( recovery->params->status == STATUS_FIND_OFFSET )
	{
		gtk_progress_bar_set_fraction ( prg_bar, (double)( recovery->params->file_nbr ) / 100 );
	}
	else
	{
		double val = (double)( ( recovery->params->offset - partition->part_offset ) * 100 / partition->part_size );
		gtk_progress_bar_set_fraction ( prg_bar, val / 100 );
	}
}

void recovery_file_sector ( uint ind, const char *file, uint64_t sz, uint64_t sector, Recovery *recovery )
{
	g_message ( "%s:: Not Realize ", __func__ );
}

gboolean recovery_done ( Recovery *recovery )
{
	return recovery->recovery_done;
}

void recovery_stop ( Recovery *recovery )
{
	recovery->stop_recovery = TRUE;
}

void recovery_search ( uint8_t ext2, uint8_t free_space, const char *dir, gboolean search_only, GtkWindow *window, Recovery *recovery )
{
	if ( recovery->selt_disk == NULL || recovery->selt_part == NULL ) { recovery_no_disk_warn ( window, recovery ); return; }

	if ( !recovery->recovery_done ) { recovery_stop ( recovery ); return; }

	recovery->params->recup_dir = g_strconcat ( dir, "/", DEFAULT_RECUP_DIR, NULL );
	recovery->params->carve_free_space_only = free_space;
	recovery->params->disk = recovery->selt_disk;
	recovery->params->partition = recovery->selt_part;

	log_partition ( recovery->selt_disk, recovery->selt_part );

	recovery->search_only = search_only;
	recovery->options->mode_ext2 = ext2;

	alloc_data_t list_search_space;
	TD_INIT_LIST_HEAD ( &list_search_space.list );

	if ( td_list_empty ( &list_search_space.list ) )
		init_search_space ( &list_search_space, recovery->params->disk, recovery->params->partition );

	if ( recovery->params->carve_free_space_only > 0 )
		recovery->params->blocksize = remove_used_space ( recovery->params->disk, recovery->params->partition, &list_search_space );

	recovery->recovery_done = FALSE;
	recovery->stop_recovery = FALSE;

	photorec ( &list_search_space, recovery );

	recovery->recovery_done = TRUE;

	free ( recovery->params->recup_dir );
	recovery->params->recup_dir = NULL;
}

void recovery_set_list_part ( GtkTreeView *treeview, Recovery *recovery )
{
	if ( recovery->list_disk == NULL ) return;

	list_part_t *element;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( treeview );

	gtk_list_store_clear ( GTK_LIST_STORE ( model ) );

	for ( element = recovery->list_part; element != NULL; element = element->next )
	{
    	const partition_t *pt = element->part;

		if ( pt->status == STATUS_EXT_IN_EXT ) continue;

			const arch_fnct_t *arch=pt->arch;
			gboolean is_n = FALSE, is_f = FALSE;
			char bufa [1024], bufn[256], buff[256];

			if ( pt->partname[0] != '\0' ) { sprintf ( bufn, "%s", pt->partname ); is_n = TRUE; }
			if ( pt->fsname[0]   != '\0' ) { sprintf ( buff, "%s", pt->fsname   ); is_f = TRUE; }

			sprintf ( bufa, "%s %s", ( is_f ) ? buff : " ", ( is_n ) ? bufn : " " );

			char buft[8]; buft[0] = '\0';
			sprintf ( buft, " %c ", get_partition_status ( pt ) );

			char *sizeinfo = g_format_size ( pt->part_size );

			gtk_list_store_append ( GTK_LIST_STORE ( model ), &iter );
			gtk_list_store_set    ( GTK_LIST_STORE ( model ), &iter,
							0, ( pt->order == NO_ORDER ) ? 0 : pt->order,
							1, buft,
							2, ( arch->get_partition_typename ( pt ) != NULL ) ? arch->get_partition_typename ( pt ) : "Unknown",
							3, ( pt->upart_type > 0 ) ? arch_none.get_partition_typename(pt) : " ",
							4, sizeinfo,
							5, bufa,
							-1 );

			free ( sizeinfo );
	}
}

void recovery_part_changed ( uint num, uint order, Recovery *recovery )
{
	uint n = 0;
	list_part_t *tmp;

	for ( tmp = recovery->list_part; tmp != NULL; tmp = tmp->next )
	{
		partition_t *part = tmp->part;

		if ( part->status == STATUS_EXT_IN_EXT ) continue;

		if ( part->order == NO_ORDER )
			{ if ( n == num ) recovery->selt_part = part; }
		else
			{ if ( part->order == order ) { recovery->selt_part = part; break; } }

		n++;
	}
}

static void recovery_set_disk ( disk_t *disk, Recovery *recovery )
{
	if( disk == NULL ) return;

	recovery->selt_disk = disk;
	recovery->selt_part = NULL;

	autodetect_arch ( recovery->selt_disk, &arch_none );

	log_info ( "%s\n", recovery->selt_disk->description_short ( recovery->selt_disk ) );

	part_free_list ( recovery->list_part );
	recovery->list_part = init_list_part ( recovery->selt_disk, NULL );

	/* If only whole disk is listed, select it */
	/* If there is the whole disk and only one partition, select the partition */
	if ( recovery->list_part != NULL )
	{
		if ( recovery->list_part->next == NULL )
			recovery->selt_part = recovery->list_part->part;
		else if ( recovery->list_part->next->next == NULL )
			recovery->selt_part = recovery->list_part->next->part;
	}

	log_all_partitions ( recovery->selt_disk, recovery->list_part );
}

gboolean recovery_add_disk ( const char *disk, Recovery *recovery )
{
	gboolean ret = FALSE;

	disk_t *new_disk = NULL;

    recovery->list_disk = insert_new_disk_aux ( recovery->list_disk, file_test_availability ( disk, recovery->options->verbose, 
		TESTDISK_O_RDONLY | TESTDISK_O_READAHEAD_32K ), &new_disk );

	if ( new_disk != NULL ) { recovery_set_disk ( new_disk, recovery ); ret = TRUE; }

	return ret;
}

void recovery_disk_changed ( uint num, Recovery *recovery )
{
	uint i = 0;
	list_disk_t *element_disk;

	for ( element_disk = recovery->list_disk, i = 0; element_disk != NULL; element_disk = element_disk->next, i++ )
	{
		if ( i == num )
		{
			recovery_set_disk ( element_disk->disk, recovery );

			return;
		}
	}
}

void recovery_set_list_disk ( GtkComboBoxText *combo, Recovery *recovery )
{
	int i = 0;
	list_disk_t *element_disk;

	gtk_combo_box_text_remove_all ( combo );

	for ( element_disk = recovery->list_disk, i = 0; element_disk != NULL; element_disk = element_disk->next, i++ )
	{
		disk_t *disk = element_disk->disk;

    	const char *text = disk->description_short ( disk );
		gtk_combo_box_text_append_text ( combo, text );

		if ( disk == recovery->selt_disk ) gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), i );
	}

	if ( !i ) { gtk_combo_box_text_append_text ( combo, "None" ); gtk_combo_box_set_active ( GTK_COMBO_BOX ( combo ), i ); }

	gtk_combo_box_text_append_text ( combo, "Add disk" );
}

static void recovery_save_reset_restore_list_formats ( uint8_t num, GtkTreeView *treeview, Recovery *recovery )
{
	file_enable_t *file_enable;

	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model ( treeview );

	if ( !gtk_tree_model_get_iter_first ( model, &iter ) ) return;

	for ( file_enable = array_file_enable; file_enable->file_hint != NULL; file_enable++ )
	{
		gboolean set = ( file_enable->file_hint->enable_by_default ) ? TRUE : FALSE;

		if ( num == SG_SAVE ) { gtk_tree_model_get ( model, &iter, 1, &set, -1 ); file_enable->enable = ( set ) ? 1 : 0; }
		if ( num == SG_RESET ) gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, 1, FALSE, -1 );
		if ( num == SG_RESTORE ) gtk_list_store_set ( GTK_LIST_STORE ( model ), &iter, 1, set, -1 );

		if ( !gtk_tree_model_iter_next ( model, &iter ) ) break;
	}
}

static void recovery_formats_win_handler ( FormatsWin *frmt, uint8_t num, GObject *obj, Recovery *recovery )
{
	GtkTreeView *treeview = GTK_TREE_VIEW ( obj );

	recovery_save_reset_restore_list_formats ( num, treeview, recovery );
}

void recovery_formats_win ( Recovery *recovery )
{
	FormatsWin *frmt = formats_win_new ();
	g_signal_connect ( frmt, "formats-set-data", G_CALLBACK ( recovery_formats_win_handler ), recovery );

	uint i = 1;
	file_enable_t *file_enable;

	for ( file_enable = array_file_enable; file_enable->file_hint != NULL; file_enable++ )
	{
		char descr[128];
		sprintf ( descr, "%-4s %s", ( file_enable->file_hint->extension != NULL ? file_enable->file_hint->extension : "" ), file_enable->file_hint->description );

		gboolean set = ( file_enable->enable ) ? TRUE : FALSE;
		formats_win_treeview_append ( i++, set, descr, frmt );
	}
}

static void recovery_init_disk ( Recovery *recovery )
{
	list_disk_t *element_disk;

	const int verbose = 0;
	const int testdisk_mode = TESTDISK_O_RDONLY | TESTDISK_O_READAHEAD_32K;

	recovery->list_disk = hd_parse ( NULL, verbose, testdisk_mode );

	hd_update_all_geometry ( recovery->list_disk, verbose );

	/* Activate the cache, even if photorec has its own */
	for ( element_disk = recovery->list_disk; element_disk != NULL; element_disk = element_disk->next )
		element_disk->disk = new_diskcache ( element_disk->disk, (uint)testdisk_mode );

	if ( recovery->list_disk ) recovery_set_disk ( recovery->list_disk->disk, recovery );
}

static void recovery_init ( Recovery *recovery )
{
	recovery->list_disk = NULL;
	recovery->selt_disk = NULL;
	recovery->list_part = NULL;
	recovery->selt_part = NULL;

    recovery->params = ( struct ph_param * )MALLOC ( sizeof ( *recovery->params ) );
    recovery->params->recup_dir = NULL;
    recovery->params->cmd_device = NULL;
    recovery->params->cmd_run = NULL;
    recovery->params->carve_free_space_only = 1;
    recovery->params->disk = NULL;
    recovery->params->partition = NULL;

    recovery->options = ( struct ph_options * )MALLOC ( sizeof ( *recovery->options ) );
    recovery->options->paranoid = 1;
    recovery->options->keep_corrupted_file = 0;
    recovery->options->mode_ext2 = 0;
    recovery->options->expert = 0;
    recovery->options->lowmem = 0;
    recovery->options->verbose = 0;
    recovery->options->list_file_format = array_file_enable;
	reset_array_file_enable ( recovery->options->list_file_format );

	recovery->search_only = FALSE;
	recovery->recovery_done = TRUE;
	recovery->stop_recovery = FALSE;

	recovery_init_disk ( recovery );
}

static void recovery_finalize ( GObject *object )
{
	Recovery *recovery = RECOVERY_OBJECT ( object );

	free ( recovery->params  );
	free ( recovery->options );

	G_OBJECT_CLASS ( recovery_parent_class )->finalize ( object );
}

static void recovery_class_init ( RecoveryClass *class )
{
	GObjectClass *oclass = G_OBJECT_CLASS (class);

	oclass->finalize = recovery_finalize;

	g_signal_new ( "recovery-set-file", G_TYPE_FROM_CLASS ( class ), G_SIGNAL_RUN_FIRST, 0, NULL, NULL, NULL, 
		G_TYPE_NONE, 4, G_TYPE_UINT, G_TYPE_STRING, G_TYPE_UINT64, G_TYPE_UINT64 );
}

Recovery * recovery_new ( void )
{
	Recovery *recovery = g_object_new ( RECOVERY_TYPE_OBJECT, NULL );

	return recovery;
}
