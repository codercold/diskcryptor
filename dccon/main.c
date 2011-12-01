/*
    *
    * DiskCryptor - open source partition encryption tool
	* Copyright (c) 2008 
	* ntldr <ntldr@freed0m.org> PGP key ID - 0xC48251EB4F8E4E6E
    *

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include "defines.h"
#include "main.h"
#include "version.h"
#include "boot_menu.h"
#include "sys\driver.h"
#include "dcapi\misc.h"
#include "dcapi\mbrinst.h"
#include "dcapi\drv_ioctl.h"
#include "dcapi\drvinst.h"
#include "dcapi\shrink.h"
#include "dcapi\rand.h"
#include "boot\boot.h"
#include "crypto\crypto.h"
#include "crypto\pkcs5.h"

typedef struct _bench_item {
	wchar_t *alg;
	wchar_t *mode;
	double   speed;

} bench_item;

       vol_inf   volumes[MAX_VOLUMES];
       u32       vol_cnt;
	   int       g_argc;
	   wchar_t **g_argv;
static int       rng_inited;
static wchar_t   boot_dev[MAX_PATH];

static void print_usage()
{
	wprintf(
		L"dccon [-key] [params]\n\n"
		L"  key:\n"
		L"   -version                      display DiskCryptor version\n"
		L"   -install                      install DiskCryptor driver\n"
		L"   -remove                       uninstall DiskCryptor driver\n"
		L"   -update                       update DiskCryptor driver\n"
		L"   -enum                         enum all volume devices in system\n"
		L"   -info    [device]             display information about device\n"
		L"   -mount   [device] [params]    mount encrypted device\n"		
		L"      -mp [mount point]   add volume mount point\n"
		L"      -p  [password]      get password from command line\n"
		L"   -mountall                     mount all encrypted devices\n"
		L"      -p  [password]      get password from command line\n"
		L"   -unmount [device] [-f]        unmount encrypted device\n"
		L"      -f       force unmount with close all opened files\n"
		L"      -dp      delete volume mount point\n"
		L"   -unmountall                   force unmount all devices\n"
		L"   -clean                        wipe cached passwords in memory\n"
		L"   -encrypt [device] [params] encrypt volume device\n"
		L"      -p  [password]      get password from command line\n"
		L"     Cipher settings:\n"
		L"      -aes                 AES cipher\n"
		L"      -twofish             Twofish cipher\n"
		L"      -serpent             Serpent cipher\n"
		L"      -aes-twofish         AES-Twofish ciphers chain\n"
		L"      -twofish-serpent     Twofish-Serpent ciphers chain\n"
		L"      -serpent-aes         Serpent-AES ciphers chain\n"
		L"      -aes-twofish-serpent AES-Twofish-Serpent ciphers chain\n"
		L"     Encryption mode:\n"
		L"      -xts xts mode (recommended)\n"
		L"      -lrw lrw mode\n"
		L"     Key derivation PRF:\n"
		L"      -sha1    HMAC-SHA-1 PRF   (2000 iterations)\n"
		L"      -sha512  HMAC-SHA-512 PRF (1000 iterations)\n"
		L"     Original data wipe settings:\n"
		L"      -dod_e   US DoD 5220.22-M (8-306. / E)          (3 passes)\n"
		L"      -dod     US DoD 5220.22-M (8-306. / E, C and E) (7 passes)\n"
		L"      -gutmann Gutmann mode                           (35 passes)\n"
		L"   -decrypt [device]             decrypt volume device\n"
		L"      -p  [password]      get password from command line\n"
		L"   -reencrypt [device] [params]  re-encrypt encrypted device with new parameters\n"
		L"                                 all parameters are similar to -encrypt\n"
		L"   -chpass  [device] [pkcs5-prf] change volume password and pkcs5.2 PRF function\n"
		L"      -sha1    HMAC-SHA-1 PRF   (2000 iterations)\n"
		L"      -sha512  HMAC-SHA-512 PRF (1000 iterations)\n"
		L"   -updvol  [device]             update volume to last volume format\n"
		L"      -p  [password]      get password from command line\n"
		L"   -format  [device] [params]    format volume device with encryption\n"
		L"                                 encryption parameters are similar to -encrypt\n"
		L"      -q     quick format\n"
		L"      -fat   format to FAT file system\n"
		L"      -fat32 format to FAT32 file system\n"
		L"      -ntfs  format to NTFS file system\n"
		L"      -raw   file system does not needed\n"
		L"   -backup  [device] [file]      backup volume header to file\n"
		L"      -p  [password]      get password from command line\n"
		L"   -restore [device] [file]      restore volume header from file\n"
		L"      -p  [password]      get password from command line\n"
		L"   -benchmark                    encryption benchmark\n"
		L"   -config                       change program configuration\n"
		L"   -bsod                         erase all keys in memory and generate BSOD\n"
		L"   -boot [action]\n"
		L"      -enum                      enumerate all HDDs\n"
		L"      -setmbr   [hdd]            setup bootloader to HDD master boot record\n"
		L"      -delmbr   [hdd]            delete bootloader from HDD master boot record\n"
		L"      -updmbr   [hdd]            update bootloader on HDD master boot record\n"
		L"      -setpar   [partition root] setup bootloader to bootable partition (Floppy, USB-Stick, etc)\n"
	    L"      -makeiso  [file]           make .iso bootloader image\n"
		L"      -makepxe  [file]           make bootloader image for PXE network booting\n"
		L"      -config   [hdd/file]       change bootloader configuration\n"
		);
}

void cls_console() 
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	COORD                      pos;
	u32                        bytes;
	HANDLE                     console = GetStdHandle(STD_OUTPUT_HANDLE);

	GetConsoleScreenBufferInfo(console, &csbi);

	pos.X = 0; pos.Y = 0;
	FillConsoleOutputCharacter(
		console, ' ', csbi.dwSize.X * csbi.dwSize.Y, pos, &bytes
		);
	SetConsoleCursorPosition(console, pos); 
}

char getchr(char min, char max) 
{
	char ch;

	do
	{
		ch = _getch();
	} while ( (ch < min) || (ch > max) );

	return ch;
}

static void make_dev_status(vol_inf *inf, wchar_t *status)
{
	wcscpy(status, L"unmounted");

	if (inf->status.flags & F_ENABLED) {
		wcscpy(status, L"mounted");
	}

	if (inf->status.flags & F_UNSUPRT) {
		wcscpy(status, L"unsupported");
	}

	if (wcscmp(inf->device, boot_dev) == 0) {
		wcscat(status, L", boot");
	}

	if (inf->status.flags & F_SYSTEM) {
		wcscat(status, L", system");
	}
}

static void print_devices()
{
	wchar_t  stat[MAX_PATH];
	wchar_t  size[MAX_PATH];
	wchar_t *mnt;
	u32      i;

	wprintf(
		L"------------------------------------------------------------------\n"
		L"volume |     mount point      |   size  |       status\n"
		L"-------+----------------------+---------+-------------------------\n"
		);

	for (i = 0; i < vol_cnt; i++)
	{
		dc_format_byte_size(
			size, sizeof_w(size), volumes[i].status.dsk_size
			);

		make_dev_status(&volumes[i], stat);

		if (volumes[i].status.mnt_point[0] != L'\\') {
			mnt = volumes[i].status.mnt_point;
		} else mnt = L"";
		
		wprintf(L"pt%d    | %-20s | %-7s | %-23s\n", i, mnt, size, stat);
	}
}

static void enum_devices()
{
	vol_inf info;
	
	if (dc_first_volume(&info) == ST_OK)
	{
		do
		{
			volumes[vol_cnt++] = info;
		} while (dc_next_volume(&info) == ST_OK);
	}
}

static vol_inf *find_device(wchar_t *name)
{
	wchar_t w_name[MAX_PATH];
	u32     idx;

	wcscpy(w_name, name); _wcslwr(w_name);

	if ( (w_name[0] == L'p') && (w_name[1] == L't') && (isdigit(name[2]) != 0) )
	{
		if ( (idx = _wtoi(name+2)) < vol_cnt ) {
			return &volumes[idx];
		}
	} else 
	{
		if (w_name[1] == L':') {
			w_name[2] = 0;
		}

		for (idx = 0; idx < vol_cnt; idx++) 
		{
			if (_wcsicmp(w_name, volumes[idx].status.mnt_point) == 0) {
				return &volumes[idx];
			}
		}
	}

	return NULL;
}

static char* dc_getpass_loop()
{
	u8 *pass, ch;
	u32 pos;

	if ( (pass = secure_alloc(MAX_PASSWORD)) == NULL ) {
		return NULL;
	}

	for (pos = 0;;)
	{
		ch = _getch();

		if (ch == '\r') {
			break;
		}

		/* reseed RNG */
		if (rng_inited != 0) {
			rnd_reseed_now();
		}

		if (ch == 8)
		{
			if (pos > 0) {
				_putch(8);
				pos--;
			}
			_putch(' '); _putch(8);
			continue;
		}

		if ( (ch == 0) || (ch == 0xE0) ) {
			_getch();
		}

		if ( (ch < ' ') || (ch > '~') || (pos == MAX_PASSWORD) ) {
			continue;
		}

		pass[pos++] = ch; _putch('*');
	}

	if (pos != 0) {
		pass[pos] = 0;
	} else {
		secure_free(pass); pass = NULL;
	}

	_putch('\n');

	return pass;
}

char* dc_get_password(int confirm)
{
	char *pass1;
	char *pass2;

	if (pass1 = dc_getpass_loop())
	{
		if (confirm != 0) 
		{
			wprintf(L"Confirm password: ");

			pass2 = dc_getpass_loop();

			if ( (pass2 == NULL) || (strcmp(pass1, pass2) != 0) ) 
			{
				wprintf(L"The password was not correctly confirmed.\n");
				secure_free(pass1); pass1 = NULL;
			}

			if (pass2 != NULL) {
				secure_free(pass2);
			}
		}
	}

	return pass1;
}

static int dc_encrypt_loop(vol_inf *inf, int wp_mode)
{
	dc_status status;
	int       i = 0;
	wchar_t  *wp_str;
	char      ch;
	int       resl;

bgn_loop:;
	cls_console();

	switch (wp_mode)
	{
		case WP_NONE: wp_str = L"None"; break;
		case WP_DOD_E: wp_str = L"US DoD 5220.22-M (8-306. / E) (3 passes)"; break;
		case WP_DOD: wp_str = L"US DoD 5220.22-M (8-306. / E, C and E) (7 passes)"; break;
		case WP_GUTMANN: wp_str = L"Gutmann (35 passes)"; break;
	}
	
	wprintf(
		L"Encrypting progress...\n"
		L"Old data wipe mode: %s\n\n"
		L"Press ESC to cancel encrypting or press \"W\" to change wipe mode\n", wp_str
		);

	do
	{
		if (_kbhit() != 0)
		{
			if ( (ch = _getch()) == 0x1B )
			{
				wprintf(L"\nEncryption cancelled\n");
				dc_sync_enc_state(inf->device);
				break;
			}

			if (tolower(ch) == 'w')
			{
				wprintf(L"\n"
					L"1 - None (fastest)\n"
					L"2 - US DoD 5220.22-M (8-306. / E) (3 passes)\n"
					L"3 - US DoD 5220.22-M (8-306. / E, C and E) (7 passes)\n"
					L"4 - Gutmann (35 passes)\n"
					);

				switch (getchr('1', '4'))
				{
					case '1': wp_mode = WP_NONE; break;
					case '2': wp_mode = WP_DOD_E; break;
					case '3': wp_mode = WP_DOD; break;
					case '4': wp_mode = WP_GUTMANN; break;
				}

				goto bgn_loop;
			}
		}

		if (i-- == 0) {
			dc_sync_enc_state(inf->device); i = 20;
		}

		dc_get_device_status(
			inf->device, &status
			);

		wprintf(
			L"\r%-.3f %%", 
			(double)(status.tmp_size) / (double)(status.dsk_size) * 100
			);

		resl = dc_enc_step(inf->device, wp_mode);

		if (resl == ST_FINISHED) {
			wprintf(L"\nEncryption finished\n");
			break;
		}

		if ( (resl != ST_OK) && (resl != ST_RW_ERR) ) {
			wprintf(L"\nEncryption error %d\n", resl);
			break;
		}
	} while (1);

	return ST_OK;
}

static int dc_format_loop(vol_inf *inf, int wp_mode)
{
	dc_status status;
	wchar_t  *wp_str;
	char      ch;
	int       resl;

bgn_loop:;
	cls_console();

	switch (wp_mode)
	{
		case WP_NONE: wp_str = L"None"; break;
		case WP_DOD_E: wp_str = L"US DoD 5220.22-M (8-306. / E) (3 passes)"; break;
		case WP_DOD: wp_str = L"US DoD 5220.22-M (8-306. / E, C and E) (7 passes)"; break;
		case WP_GUTMANN: wp_str = L"Gutmann (35 passes)"; break;
	}
	
	wprintf(
		L"Formatting progress...\n"
		L"Old data wipe mode: %s\n\n"
		L"Press ESC to cancel encrypting or press \"W\" to change wipe mode\n", wp_str
		);

	do
	{
		if (_kbhit() != 0)
		{
			if ( (ch = _getch()) == 0x1B )
			{
				wprintf(L"\nFormatting cancelled\n");
				resl = ST_OK; break;
			}

			if (tolower(ch) == 'w')
			{
				wprintf(L"\n"
					L"1 - None (fastest)\n"
					L"2 - US DoD 5220.22-M (8-306. / E) (3 passes)\n"
					L"3 - US DoD 5220.22-M (8-306. / E, C and E) (7 passes)\n"
					L"4 - Gutmann (35 passes)\n"
					);

				switch (getchr('1', '4'))
				{
					case '1': wp_mode = WP_NONE; break;
					case '2': wp_mode = WP_DOD_E; break;
					case '3': wp_mode = WP_DOD; break;
					case '4': wp_mode = WP_GUTMANN; break;
				}

				goto bgn_loop;
			}
		}

		dc_get_device_status(inf->device, &status);

		wprintf(
			L"\r%-.3f %%", 
			(double)(status.tmp_size) / (double)(status.dsk_size) * 100
			);

		resl = dc_format_step(inf->device, wp_mode);

		if (resl == ST_FINISHED) {
			_putch('\n'); break;
		}

		if ( (resl != ST_OK) && (resl != ST_RW_ERR) ) {
			wprintf(L"\nFormatting error %d\n", resl);
			break;
		}
	} while (1);

	if (resl != ST_FINISHED) {
		dc_done_format(inf->device);
	}

	return ST_OK;
}

static int dc_decrypt_loop(vol_inf *inf)
{
	dc_status status;
	int       i = 0;
	int       resl;

	cls_console();
	
	wprintf(
		L"Decrypting progress...\n"
		L"Press ESC to cancel decrypting\n"
		);

	do
	{
		if ( (_kbhit() != 0) && (_getch() == 0x1B) ) {
			wprintf(L"\nDecryption cancelled\n");
			dc_sync_enc_state(inf->device);
			break;
		}

		if (i-- == 0) {
			dc_sync_enc_state(inf->device); i = 20;					
		}

		dc_get_device_status(
			inf->device, &status
			);

		wprintf(
			L"\r%-.3f %%", 
			100 - ((double)(status.tmp_size) / (double)(status.dsk_size) * 100)
			);

		resl = dc_dec_step(inf->device);

		if (resl == ST_FINISHED) {
			wprintf(L"\nDecryption finished\n");
			break;
		}

		if ( (resl != ST_OK) && (resl != ST_RW_ERR) ) {
			wprintf(L"\nDecryption error %d\n", resl);
			break;
		}
	} while (1);

	return ST_OK;
}


int dc_set_boot_interactive(int d_num)
{
	ldr_config conf;
	int        resl;

	if ( (resl = dc_set_mbr(d_num, 0)) == ST_NF_SPACE )
	{
		wprintf(
			L"Not enough space after partitions to install bootloader.\n"
			L"Install bootloader to first HDD track (incompatible with third-party bootmanagers, like GRUB) Y/N?\n"
			);

		if (tolower(_getch()) == 'y') 
		{
			if ( ((resl = dc_set_mbr(d_num, 1)) == ST_OK) && 
				 (dc_get_mbr_config(d_num, NULL, &conf) == ST_OK) )
			{
				conf.boot_type = BT_ACTIVE;
						
				if ( (resl = dc_set_mbr_config(d_num, NULL, &conf)) != ST_OK ) {
					dc_unset_mbr(d_num);
				}
			}
		} 
	}

	return resl;
}

static 
int dc_shrink_callback(
	  int stage, vol_inf *inf, wchar_t *file, int status
	  )
{
	if (stage == SHRINK_BEGIN) 
	{
		wprintf(
			L"Last two sectors on volume %s are used\n"
			L"Shrinking volume... (press ESC to cancel)\n",
			(inf->status.mnt_point[0] == 0) ? inf->device : inf->status.mnt_point
			);
	}

	if (stage == SHRINK_STEP)
	{
		if ( (_kbhit() != 0) && (_getch() == 0x1B) ) {
			return ST_ERROR;
		}
	}

	if (stage == SHRINK_END)
	{
		if (status == ST_OK) {
			wprintf(L"Volume shrinked successfully\n");
		} else {
			wprintf(L"Volume not shrinked, error %d\n", status);
		}
	}

	return ST_OK;
}

static int is_param(wchar_t *name)
{
	int i;
	
	for (i = 0; i < g_argc; i++)
	{
		if (_wcsicmp(g_argv[i], name) == 0) {
			return 1;
		}
	}

	return 0;
}

static char *cmd_line_pass() 
{
	wchar_t *cmd_w = GetCommandLineW();
	char    *cmd_a = GetCommandLineA();
	char    *pass = NULL;
	wchar_t *cmp;
	int      i, found = 0;

	for (i = 0; i < g_argc - 1; i++)
	{
		if (_wcsicmp(g_argv[i], L"-p") == 0) 
		{
			cmp = g_argv[i+1];
			if (pass = secure_alloc(MAX_PASSWORD + 1)) {
				wcstombs(pass, cmp, MAX_PASSWORD);
			}
			zeromem(cmp, wcslen(cmp) * sizeof(wchar_t));
			found = 1; break;
		}		
	}

	/* clear command line in PEB */
	if (found != 0) {
		zeromem(cmd_w, wcslen(cmd_w) * sizeof(wchar_t));
		zeromem(cmd_a, strlen(cmd_a) * sizeof(char));	
	}

	return pass;
}

char* dc_load_pass(int confirm)
{
	char *pass;

	if (pass = cmd_line_pass()) {
		return pass;
	}

	wprintf(L"Enter password: ");

	return dc_get_password(confirm);
}

static void get_crypt_info(crypt_info *crypt)
{
	/* get cipher */
	if (is_param(L"-aes") != 0) {
		crypt->cipher_id = CF_AES;
	} else if (is_param(L"-twofish") != 0) {
		crypt->cipher_id = CF_TWOFISH;
	} else if (is_param(L"-serpent") != 0) {
		crypt->cipher_id = CF_SERPENT;
	} else if (is_param(L"-aes-twofish") != 0) {
		crypt->cipher_id = CF_AES_TWOFISH;
	} else if (is_param(L"-twofish-serpent") != 0) {
		crypt->cipher_id = CF_TWOFISH_SERPENT;
	} else if (is_param(L"-serpent-aes") != 0) {
		crypt->cipher_id = CF_SERPENT_AES;
	} else if (is_param(L"-aes-twofish-serpent") != 0) {
		crypt->cipher_id = CF_AES_TWOFISH_SERPENT;
	}

	/* get encryption mode */
	if (is_param(L"-xts") != 0) {
		crypt->mode_id = EM_XTS;
	} else if (is_param(L"-lrw") != 0) {
		crypt->mode_id = EM_LRW;
	}

	/* get PRF */
	if (is_param(L"-sha512") != 0) {
		crypt->prf_id = PRF_HMAC_SHA512;
	} else if (is_param(L"-sha1") != 0) {
		crypt->prf_id = PRF_HMAC_SHA1;
	}

	/* get wipe mode */
	if (is_param(L"-dod_e") != 0) {
		crypt->wp_mode = WP_DOD_E;
	} else if (is_param(L"-dod") != 0) {
		crypt->wp_mode = WP_DOD;
	} else if (is_param(L"-gutmann") != 0) {
		crypt->wp_mode = WP_GUTMANN;
	}
}


static int dc_bench_cmp(const bench_item *arg1, const bench_item *arg2)
{
	if (arg1->speed > arg2->speed) {
		return -1;
	} else {
		return (arg1->speed < arg2->speed);
	}
}
 
int wmain(int argc, wchar_t *argv[])
{
	vol_inf *inf;
	int      resl;
	int      status;
	int      vers;
	int      d_inited;

	g_argc = argc; g_argv = argv;
	do
	{
#ifdef _M_IX86 
		if (is_wow64() != 0) {
			wprintf(L"Please use x64 version of DiskCryptor\n");
			resl = ST_ERROR; break;
		}
#endif
		if (is_admin() != ST_OK) {
			wprintf(L"Administrator privilegies required\n");
			resl = ST_NO_ADMIN; break;
		}

		if (argc < 2) {
			print_usage();
			resl = ST_OK; break;
		}

		/* get driver load status */
		status   = dc_driver_status();
		d_inited = 0;

		if ( (status == ST_OK) && (dc_open_device() == ST_OK) )
		{
			if ((vers = dc_get_version()) == DC_DRIVER_VER) {
				/* get information of all volumes in system */
				enum_devices(); d_inited = 1;
			}
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-version") == 0) ) 
		{
			wprintf(L"DiskCryptor %s console\n", DC_FILE_VER);
			resl = ST_OK; break;
		}		

		if ( (argc == 2) && (wcscmp(argv[1], L"-install") == 0) ) 
		{
			if (status != ST_ERROR) 
			{
				wprintf(L"DiskCryptor driver already installed\n");

				if (status != ST_OK) {
					wprintf(L"Please reboot you system\n");
				}
				resl = ST_OK; break;
			}

			if ( (resl = dc_install_driver(NULL)) == ST_OK ) {
				wprintf(L"DiskCryptor driver installed, please reboot you system\n");
			}
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-remove") == 0) ) 
		{
			if (status == ST_ERROR) {
				wprintf(L"DiskCryptor driver not installed\n");
				resl = ST_OK; break;
			}

			if ( (resl = dc_remove_driver(NULL)) == ST_OK ) {
				wprintf(L"DiskCryptor driver uninstalled, please reboot you system\n");				
			}
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-update") == 0) ) 
		{
			if (status == ST_ERROR) {
				wprintf(L"DiskCryptor driver not installed\n");
				resl = ST_ERROR; break;
			}

			if ( (resl = dc_update_boot(-1)) == ST_OK ) {
				wprintf(L"DiskCryptor bootloader updated\n");
			} else if (resl != ST_BLDR_NOTINST) {
				wprintf(L"Bootloader update error, please update it manually\n");
				break;
			}

			if ( (resl = dc_update_driver()) == ST_OK ) {
				wprintf(L"DiskCryptor driver updated, please reboot you system\n");				
			}
			break;			
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-boot") == 0) ) 
		{
			resl = boot_menu(argc, argv);
			break;
		}	

		if (status != ST_OK) 
		{
			wprintf(
				L"DiskCryptor driver not installed.\n"
				L"please run \"dccon -install\" and reboot you system\n"
				);
			resl = ST_OK; break;
		}

		if (d_inited == 0)
		{
			if (vers > DC_DRIVER_VER) 
			{
				wprintf(
					L"DiskCryptor driver ver %d detected\n"
					L"Please use last program version\n", vers
					);
				resl = ST_OK; break;
			}

			if (vers < DC_DRIVER_VER)
			{
				wprintf(
					L"Old DiskCryptor driver detected\n"
					L"please run \"dccon -update\" and reboot you system\n"
					);
				resl = ST_OK; break;
			}

			resl = ST_ERROR; break;
		}

		/* initialize user mode RNG part */
		if ( (resl = rnd_init()) != ST_OK ) {
			break;
		}
		rng_inited = 1;

		/* get boot device */
		if (dc_get_boot_device(boot_dev) != ST_OK) {
			boot_dev[0] = 0;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-enum") == 0) ) {
			print_devices();
			resl = ST_OK; break;
		}

		if ( (argc == 3) && (wcscmp(argv[1], L"-info") == 0) ) 
		{
			wchar_t stat[MAX_PATH];
			wchar_t size[MAX_PATH];

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			dc_format_byte_size(
				size, sizeof_w(size), inf->status.dsk_size
				);

			make_dev_status(inf, stat);

			wprintf(
				L"Device:            %s\n"
				L"SymLink:           %s\n"
				L"Mount point:       %s\n"
				L"Capacity:          %s\n"
				L"Status:            %s\n",
				inf->device, inf->w32_device, inf->status.mnt_point,
				size, stat
				);

			if (inf->status.flags & F_ENABLED)
			{
				double portion;

				if (inf->status.flags & F_SYNC) 
				{
					portion = (double)(inf->status.tmp_size) / 
						(double)(inf->status.dsk_size) * 100;
				} else {
					portion = 100;
				}

				wprintf(
					L"Cipher:            %s\n"
					L"Encryption mode:   %s\n"
					L"Pkcs5.2 prf:       %s\n"
					L"Encrypted portion: %-.3f%%\n",
					dc_get_cipher_name(inf->status.crypt.cipher_id),
					dc_get_mode_name(inf->status.crypt.mode_id),
					dc_get_prf_name(inf->status.crypt.prf_id),
					portion
					);
			}

			resl = ST_OK; break;
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-mount") == 0) ) 
		{
			wchar_t vol_n[MAX_PATH];
			wchar_t mnt_p[MAX_PATH];
			char   *pass;
			size_t  s;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if (inf->status.flags & F_ENABLED) {
				wprintf(L"This device is already mounted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & (F_UNSUPRT | F_DISABLE | F_FORMATTING)) {
				wprintf(L"Invalid device state\n");
				resl = ST_OK; break;
			}

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			if ( (resl = dc_mount_volume(inf->device, pass)) == ST_OK ) 
			{
				if ( (argc == 5) && (_wcsicmp(argv[3], L"-mp") == 0) ) 
				{
					if (inf->status.mnt_point[0] != L'\\') {
						wprintf(L"device %s already have mount point\n", argv[2]);						
					} else 
					{
						_snwprintf(
							vol_n, sizeof_w(vol_n), L"%s\\", inf->w32_device
							);

						wcsncpy(mnt_p, argv[4], sizeof_w(mnt_p));
						if ( (s = wcslen(mnt_p)) && (mnt_p[s-1] != L'\\') ) {
							mnt_p[s] = L'\\'; mnt_p[s+1] = 0;
						}

						if (SetVolumeMountPoint(mnt_p, vol_n) == 0) {
							wprintf(L"Error when adding mount point\n");
						}
					}
				}

				wprintf(L"device %s mounted\n", argv[2]);
			}

			secure_free(pass);
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-mountall") == 0) ) 
		{
			char *pass;
			int   n_mount;

			resl = dc_mount_all(
				pass = dc_load_pass(0), &n_mount
				);

			if (resl == ST_OK) {
				wprintf(L"%d devices mounted\n", n_mount);
			}

			if (pass != NULL) {
				secure_free(pass);
			}
			break;
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-unmount") == 0) )
		{
			wchar_t mnt_p[MAX_PATH];
			int     flags = 0;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if (is_param(L"-f") != 0) {
				flags = UM_FORCE;
			}

			if ( !(inf->status.flags & F_ENABLED) ) {
				wprintf(L"This device is not mounted\n");
				resl = ST_OK; break;
			}

			resl = dc_unmount_volume(inf->device, flags);

			if (resl == ST_LOCK_ERR)
			{
				wprintf(
					L"This volume contain opened files.\n"
					L"Would you like to force a unmount on this volume? (Y/N)\n"
					);

				if (tolower(_getch()) == 'y') {
					resl = dc_unmount_volume(inf->device, UM_FORCE);
				}
			}

			if (resl == ST_OK) 
			{
				if (is_param(L"-dp") != 0) 
				{
					_snwprintf(
						mnt_p, sizeof_w(mnt_p), L"%s\\", inf->status.mnt_point
						);					

					DeleteVolumeMountPoint(mnt_p);
				}

				wprintf(L"device %s unmounted\n", argv[2]);
			}
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-unmountall") == 0) ) 
		{
			resl = dc_unmount_all();

			if (resl == ST_OK) {
				wprintf(L"all devices unmounted\n");
			}
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-clean") == 0) ) 
		{
			resl = dc_clean_pass_cache();

			if (resl == ST_OK) {
				wprintf(L"passwords has been erased in memory\n");
			}
			break;
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-encrypt") == 0) )
		{
			char      *pass;			
			sh_data    shd;
			crypt_info crypt;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			/* set default params */
			crypt.cipher_id = CF_AES;
			crypt.mode_id   = EM_XTS;
			crypt.prf_id    = PRF_HMAC_SHA512;
			crypt.wp_mode   = WP_NONE;

			get_crypt_info(&crypt);

			if (inf->status.flags & F_SYNC) 
			{
				if (crypt.wp_mode == WP_NONE) {
					crypt.wp_mode = inf->status.crypt.wp_mode;
				}

				resl = dc_encrypt_loop(inf, crypt.wp_mode);
				break;
			}

			if (inf->status.flags & F_ENABLED) {
				wprintf(L"This device is already encrypted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & (F_UNSUPRT | F_DISABLE)) {
				wprintf(L"Invalid device state\n");
				resl = ST_OK; break;
			}

			if ( (inf->status.flags & F_SYSTEM) || (wcscmp(inf->device, boot_dev) == 0) )
			{
				ldr_config conf;
				int        b_dsk;
				
				if (dc_get_boot_disk(&b_dsk) != ST_OK)
				{
					wprintf(
						L"This partition needed for system booting and bootable HDD not found\n"
						L"You must be use external bootloader\n"
						L"Continue operation (Y/N)?\n\n"
						);

					if (tolower(_getch()) != 'y') {
						resl = ST_OK; break;
					}
				} else if (dc_get_mbr_config(b_dsk, NULL, &conf) != ST_OK)
				{
					wprintf(
						L"This partition needed for system booting\n"
						L"You must install bootloader to HDD, or use external bootloader\n\n"
						L"1 - Install to HDD\n"
						L"2 - I already have external bootloader\n"
						);

					if (getchr('1', '2') == '1') 
					{
						if ( (resl = dc_set_boot_interactive(b_dsk)) != ST_OK ) {
							break;
						}
					}
				}				
			}

			if ( (pass = dc_load_pass(1)) == NULL ) {
				resl = ST_OK; break;
			}

			shd.sh_pend = (inf->status.flags & F_SYSTEM) != 0;
			shd.offset  = 0;
			shd.value   = 0;

			resl = dc_shrink_volume(
				inf->w32_device, HEADER_SIZE + DC_RESERVED_SIZE, dc_shrink_callback, inf, &shd
				);

			if (resl == ST_OK) {
				resl = dc_start_encrypt(inf->device, pass, &crypt);
			}

			secure_free(pass);

			if (resl == ST_OK ) 
			{
				if (shd.sh_pend != 0) {
					dc_set_shrink_pending(inf->device, &shd);
				}

				resl = dc_encrypt_loop(inf, crypt.wp_mode);
			}
			break;
		}

		if ( (argc == 3) && (wcscmp(argv[1], L"-decrypt") == 0) ) 
		{
			char *pass;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if (inf->status.flags & F_SYNC) {
				resl = dc_decrypt_loop(inf);
				break;
			}

			if ( !(inf->status.flags & F_ENABLED) ) {
				wprintf(L"This device is not mounted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_FORMATTING) {
				wprintf(L"Invalid device state\n");
				resl = ST_OK; break;
			}

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			resl = dc_start_decrypt(inf->device, pass);

			secure_free(pass);

			if (resl == ST_OK ) {
				resl = dc_decrypt_loop(inf);
			}
			break;
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-reencrypt") == 0) ) 
		{
			crypt_info crypt;
			char      *pass;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			crypt = inf->status.crypt;
			get_crypt_info(&crypt);

			if ( !(inf->status.flags & F_ENABLED) ) {
				wprintf(L"This device is not mounted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_FORMATTING) {
				wprintf(L"Invalid device state\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_SYNC)
			{
				if (inf->status.flags & F_REENCRYPT)
				{
					resl = dc_encrypt_loop(inf, crypt.wp_mode);
					break;
				} else 
				{
					wprintf(L"This device is not complete encrypted\n");
					resl = ST_OK; break;
				}
			}

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			resl = dc_start_re_encrypt(inf->device, pass, &crypt);

			secure_free(pass);

			if (resl == ST_OK ) {
				resl = dc_encrypt_loop(inf, crypt.wp_mode);
			}
			break;
		}		

		if ( (argc >= 3) && (wcscmp(argv[1], L"-chpass") == 0) ) 
		{
			crypt_info crypt;
			char      *old_p, *new_p;
			
			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if ( !(inf->status.flags & F_ENABLED) ) {
				wprintf(L"This device is not mounted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_SYNC) {
				wprintf(L"This device is not complete encrypted\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_FORMATTING) {
				wprintf(L"Invalid device state\n");
				resl = ST_OK; break;
			}

			crypt = inf->status.crypt;

			get_crypt_info(&crypt);

			old_p = NULL; new_p = NULL;
			do
			{
				wprintf(L"Enter old password: ");

				if ( (old_p = dc_get_password(0)) == NULL ) {
					resl = ST_OK; break;
				}

				wprintf(L"Enter new password: ");

				if ( (new_p = dc_get_password(1)) == NULL ) {
					resl = ST_OK; break;
				}

				resl = dc_change_password(
					inf->device, old_p, new_p, crypt.prf_id
					);

				if (resl == ST_OK) {
					wprintf(L"The password successfully changed\n");
				}
			} while (0);

			if (old_p != NULL) {
				secure_free(old_p);
			}

			if (new_p != NULL) {
				secure_free(new_p);
			}
			break;
		}

		if ( (argc == 3) && (wcscmp(argv[1], L"-updvol") == 0) ) 
		{
			sh_data  shd;
			char    *pass;			

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if ( !(inf->status.flags & F_ENABLED) ) {
				wprintf(L"This device is not mounted\n");
				resl = ST_OK; break;
			}

			if (inf->status.vf_version >= TC_VOLUME_HEADER_VERSION) {
				wprintf(L"This partition has already updated\n");
				resl = ST_OK; break;
			}

			if (inf->status.flags & F_SYNC) {
				wprintf(L"Partial encrypted volume can not be updated\n");
				resl = ST_OK; break;
			}

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			shd.sh_pend = (inf->status.flags & F_SYSTEM) != 0;
			shd.offset  = 0;
			shd.value   = 0;

			resl = dc_shrink_volume(
				inf->w32_device, HEADER_SIZE + DC_RESERVED_SIZE, dc_shrink_callback, inf, &shd
				);

			if (resl == ST_OK) {
				resl = dc_update_volume(inf->device, pass, &shd);
			}

			secure_free(pass);

			if (resl == ST_OK ) {
				wprintf(L"device %s updated\n", argv[2]);
			}
		}

		if ( (argc >= 3) && (wcscmp(argv[1], L"-format") == 0) ) 
		{
			crypt_info crypt;
			char      *pass;
			wchar_t   *fs;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			crypt = inf->status.crypt;
			get_crypt_info(&crypt);

			if (inf->status.flags & F_FORMATTING) {				
				resl = ST_OK;
			} else 
			{
				if (inf->status.flags & F_ENABLED) {
					wprintf(L"This device is mounted, please unmount it\n");
					resl = ST_OK; break;
				}

				if ( (pass = dc_load_pass(0)) == NULL ) {
					resl = ST_OK; break;
				}

				resl = dc_start_format(inf->device, pass, &crypt);

				secure_free(pass);
			}

			if (resl == ST_OK) 
			{
				if (is_param(L"-q") != 0) {
					resl = dc_done_format(inf->device);
				} else {
					resl = dc_format_loop(inf, crypt.wp_mode);
				}

				if (is_param(L"-ntfs") != 0) {
					fs = L"NTFS";
				} else if (is_param(L"-fat") != 0) {
					fs = L"FAT";
				} else if (is_param(L"-fat32") != 0) {
					fs = L"FAT32";
				} else fs = NULL;

				if (resl == ST_OK) 
				{
					wprintf(L"Creating file system...\n");

					if (fs != NULL) {
						resl = dc_format_fs(inf->w32_device, fs);
					}

					if (resl == ST_OK) {
						wprintf(L"Formatting successfully completed.\n");
					}
				}
			}
			break;
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-benchmark") == 0) ) 
		{
			dc_bench   info;
			crypt_info crypt;
			bench_item bench[CF_CIPHERS_NUM * EM_NUM];
			int        i, j, n = 0;			

			for (i = 0; i < CF_CIPHERS_NUM; i++)
			{
				for (j = 0; j < EM_NUM; j++)
				{
					crypt.cipher_id = i;
					crypt.mode_id   = j;

					if (dc_benchmark(&crypt, &info) != ST_OK) {
						break;
					}

					bench[n].alg   = dc_get_cipher_name(i);
					bench[n].mode  = dc_get_mode_name(j);
					bench[n].speed = (double)info.data_size / 
						( (double)info.enc_time / (double)info.cpu_freq) / 1024 / 1024;
					n++;
				}
			}

			qsort(&bench, n, sizeof(bench[0]), dc_bench_cmp);

			wprintf(
				L"---------------------+------+--------------\n"
				L"        cipher       | mode |     speed\n"
				L"---------------------+------+--------------\n"
				);			

			for (i = 0; i < n; i++) 
			{
				wprintf(
					L" %-19s | %-3s  | %-.2f mb/s\n",
					bench[i].alg, bench[i].mode, bench[i].speed
					);
			}
			resl = ST_OK; break;
		}

		if ( (argc == 4) && (wcscmp(argv[1], L"-backup") == 0) ) 
		{
			char *pass;
			u8    backup[SECTOR_SIZE];

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if (inf->status.flags & F_SYNC) {
				wprintf(L"This device is not complete encrypted\n");
				resl = ST_OK; break;
			}

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			resl = dc_backup_header(inf->device, pass, backup);

			secure_free(pass);

			if (resl == ST_OK) {
				resl = save_file(argv[3], backup, sizeof(backup));
			}

			if (resl == ST_OK) {
				wprintf(L"Volume header backup successfully saved.\n");
			}			
		}

		if ( (argc == 4) && (wcscmp(argv[1], L"-restore") == 0) ) 
		{
			char  *pass;
			u8     backup[SECTOR_SIZE];
			HANDLE hfile;
			u32    bytes;

			if ( (inf = find_device(argv[2])) == NULL ) {
				resl = ST_NF_DEVICE; break;
			}

			if (inf->status.flags & F_ENABLED) {
				wprintf(L"Please unmount device first\n");
				resl = ST_OK; break;
			}

			hfile = CreateFile(
				argv[3], GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL
				);

			if (hfile == INVALID_HANDLE_VALUE) {
				resl = ST_NF_FILE; break;
			}

			ReadFile(hfile, backup, sizeof(backup), &bytes, NULL);
			CloseHandle(hfile);

			if ( (pass = dc_load_pass(0)) == NULL ) {
				resl = ST_OK; break;
			}

			resl = dc_restore_header(inf->device, pass, backup);

			secure_free(pass);

			if (resl == ST_OK) {
				wprintf(L"Volume header successfully restored.\n");
			}
		}

		if ( (argc == 2) && (wcscmp(argv[1], L"-config") == 0) ) 
		{
			dc_conf_data dc_conf;

			if ( (resl = dc_load_conf(&dc_conf)) != ST_OK ) {
				break;
			}

			do
			{
				int  onoff;
				char ch;

				cls_console();

				wprintf(
					L"1 - On/Off passwords caching (%s)\n"
					L"2 - Save changes and exit\n\n",
					on_off(dc_conf.conf_flags & CONF_CACHE_PASSWORD)
					);

				if ( (ch = getchr('1', '2')) == '2' ) {
					break;
				}

				wprintf(L"0 - OFF\n1 - ON\n");
				onoff = (getchr('0', '1') == '1');

				if (ch == '1') {
					set_flag(dc_conf.conf_flags, CONF_CACHE_PASSWORD, onoff);
				} 
			} while (1);

			if ( (resl = dc_save_conf(&dc_conf)) == ST_OK ) {
				wprintf(L"Configuration successfully saved\n");
			}
		}	

		if ( (argc == 2) && (wcscmp(argv[1], L"-bsod") == 0) ) 
		{
			dc_get_bsod(); resl = ST_OK;
			break;
		}
	} while (0);

	if (resl != ST_OK) {
		wprintf(L"Error: %d\n", resl);
	}

	return resl;
}
