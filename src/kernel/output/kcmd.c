#include <limits.h>
#include <cpuid.h>

#include <x86_64/acpi/acpi.h>
#include <x86_64/pci.h>
#include <x86_64/pit.h>

#include <kernel/kstring.h>
#include <kernel/crash.h>

#include <output/image.h>
#include <output/kterm.h>

#include <ps2/mouse.h>
#include <ps2/kbd.h>

#include <mm/mem.h>

#include <fs/fs.h>

#define kcmd_strequal(s1, s2) (str_cmp(s1, s2) == 0)

extern uint8_t kacpi_apsrunning;

void kcmd_cd(char *kterm_argv[], int kterm_argc)
{
    if (kterm_getpartition() == -1)
    {
        kterm_putf("\nInvalid partition selection.");
        return;
    }

    kfs_partition *selected_partition = k_infotable.kfs_partitions[kterm_getpartition()];

    if ((uint64_t)selected_partition == 0)
    {
        kterm_putf("\nPartition does not exist.");
        kterm_setpartition(-1);
        return;
    }

    int check = 0;
    if (kterm_argv[1])
    {
        char *fullname = kterm_getabsolutedir(kterm_argv[1]);

        char *absolutename = kmem_kalloc(str_len(fullname) + str_len(kterm_argv[1]) + 2);
        check = kfs_checkdir(selected_partition, fullname, absolutename);
        
        if (check == 1)
            kterm_setdir(absolutename);
    }
}

void kcmd_clear()
{
    kterm_clr(kterm_getbg());
}

void kcmd_color(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc == 1)
        kterm_setcolor(0xC5C5C5, default_color);
    else if (kterm_argc == 3)
    {
        int64_t new_fg, new_bg;
        new_fg = str_atoi(kterm_argv[1]);
        new_bg = str_atoi(kterm_argv[2]);
        if (new_fg > 0xFFFFFFFF || new_fg < 0 ||
            new_bg > 0xFFFFFFFF || new_bg < 0)
            kterm_putf("\nInvalid colors.");
        else
            kterm_setcolor(new_fg, new_bg);
    }
    else
        kterm_putf("\nInvalid argument count.");
}

void kcmd_compare(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc < 3)
        kterm_putf("\nNot enough arguments.");
    else
    {
        long a = 0, b = 0;
        if (kterm_argv[1])
            a = str_atoi(kterm_argv[1]);
        if (kterm_argv[2])
            b = str_atoi(kterm_argv[2]);
        kterm_putf("\nlarger number is: ");
        if (a == b)
            kterm_putf("both numbers (%d) (%d)", a, b);
        else if (a > b)
            kterm_putf("number 1 (%d)", a);
        else
            kterm_putf("number 2 (%d)", b);
    }
}

void kcmd_crash()
{
    kterm_putf("\nInitiating crash...");
    kcrash("User Requested");
}

#include <fs/fs_fat.h>

void kcmd_dir(char *kterm_argv[], int kterm_argc)
{
    if (kterm_getpartition() == -1)
    {
        kterm_putf("\nInvalid partition selection.");
        return;
    }

    kfs_partition *selected_partition = k_infotable.kfs_partitions[kterm_getpartition()];

    if ((uint64_t)selected_partition == 0)
    {
        kterm_putf("\nPartition does not exist.");
        kterm_setpartition(-1);
        return;
    }

    if (kterm_argc < 2)
        kfs_printdir(selected_partition, kterm_getdir());
    else if (kterm_argc == 2)
    {
        if (!kterm_argv[1])
            kfs_printdir(selected_partition, kterm_getdir());
        else
        {
            char *fullname = kterm_getabsolutedir(kterm_argv[1]);
            kfs_printdir(selected_partition, fullname);
            kmem_kfree(fullname);
        }
    }
}

void kcmd_fs(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc < 2)
    {
        kfs_printinfo();
        kterm_putf("\nmounted partitions:");

        int partitions = 0;
        for (int i = 0; i < 16; i++)
        {
            if ((uint64_t)k_infotable.kfs_partitions[i])
            {
                kterm_putf("\n - partition %d:", i);
                kfs_printpartition(k_infotable.kfs_partitions[i]);
                partitions = 1;
            }
        }

        if (partitions == 0)
            kterm_putf("\n - No partitions mounted");
    }
    else if (kterm_argc == 2)
    {
        long part = 0;
        if (kterm_argv[1])
            part = str_atoi(kterm_argv[1]);
        
        if (part < 0 || part > 15)
        {
            kterm_putf("\nInvalid partition selection.");
            kterm_setpartition(-1);
        }
        else if ((uint64_t)k_infotable.kfs_partitions[part] == 0)
        {
            kterm_putf("\nPartition does not exist.");
            kterm_setpartition(-1);
        }
        else
        {
            kterm_putf("\nPartition set to %d.", part);
            kterm_setpartition(part);
            kterm_setdir("/");
        }
    }
    else
        kterm_putf("\nInvalid argument count.");
}

void kcmd_font()
{
    kterm_putf("\n");
    int c = 0;
    for (; c < 255; c += 8)
        kterm_putf("%c%c%c%c%c%c%c%c", c, c + 1, c + 2, c + 3, c + 4, c + 5, c + 6, c + 7);
}

void kcmd_help()
{
    kterm_putf("\nList of currently available commands:");
    kterm_putf("\n cd [path] - changes current directory");
    kterm_putf("\n clear - clears the screen");
    kterm_putf("\n color [fg] [bg] - set terminal colors in base10 of hex code, no values to reset");
    kterm_putf("\n compare [num1] [num2] - compares two numbers and prints out the largest");
    kterm_putf("\n crash - crashes the AQUA kernel");
    kterm_putf("\n dir - prints files and folders of cwd");
    kterm_putf("\n dir [path] - prints files and folders of path");
    kterm_putf("\n fs - provides all fs info");
    kterm_putf("\n fs [part] - sets current fs to selected partition");
    kterm_putf("\n font - prints all characters in boot font");
    kterm_putf("\n help - lists available commands");
    kterm_putf("\n image [filename] - attempts printing supported image types to screen");
    kterm_putf("\n info - prints current AQUA build information");
    kterm_putf("\n info [subcommand] - gives specific environment info");
    kterm_putf("\n read_file [filename] - attempt read of file on current partition");
    kterm_putf("\n shutdown - attempts acpi shutdown");
    kterm_putf("\n test - test random features");
    kterm_putf("\n test [subcommand] - tests specific features");
    kterm_putf("\n wait [sec] - wait given number of seconds");
}

void kcmd_image(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc < 2)
        kterm_putf("\nNot enough arguments.");

    if (kterm_getpartition() == -1)
    {
        kterm_putf("\nInvalid partition selection.");
        return;
    }

    kfs_partition *selected_partition = k_infotable.kfs_partitions[kterm_getpartition()];

    if ((uint64_t)selected_partition == 0)
    {
        kterm_putf("\nPartition does not exist.");
        kterm_setpartition(-1);
        return;
    }

    char *filename = 0;
    if (kterm_argv[1])
        filename = kterm_argv[1];

    size_t file_length;
    uint8_t *file = 0;
    size_t image_pages = 0;

    char *fullname = kterm_getabsolutedir(filename);

    file = kfs_readfile(selected_partition, fullname, &file_length);
    
    if ((uint64_t)file == 0)
    {
        kterm_putf("\nFile not found.");
        kmem_kfree(fullname);
        return;
    }

    kmem_kfree(fullname);

    uint32_t *image_pixels = 0;

    if (kimage_istga(file) == 1)
        image_pixels = kimage_getbuftga(file, (int)file_length, &image_pages);
    else
        kterm_putf("\nUnable to read file.");

    if ((uint64_t)image_pixels != 0)
    {
        kimage_termblit(image_pixels, k_infotable.k_graphics->horizontal_res / 16, 25);
        kmem_free(image_pixels, image_pages);
    }

    kmem_free(file, (file_length + 0x1000 - 1) / 0x1000);
}

void kcmd_info(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc == 2)
    {
        if (kcmd_strequal(kterm_argv[1], "cpu"))
        {
            unsigned int ax, bx, cx, dx;
            __cpuid(0, ax, bx, cx, dx);

            kterm_putf("\n - Vendor [%4s%4s%4s]", &bx, &dx, &cx);

            __cpuid(0x80000000, ax, bx, cx, dx);
            if (ax >= 0x80000004)
            {
                uint32_t* name = kmem_alloc(1);
                __cpuid(0x80000002, name[0], name[1], name[2], name[3]);
                __cpuid(0x80000003, name[4], name[5], name[6], name[7]);
                __cpuid(0x80000004, name[8], name[9], name[10], name[11]);
                name[12] = 0;
                
                str_trim((char *)name);
                kterm_putf(" Brand [%s]", name);
                kmem_free(name, 1);
            }

            __cpuid(1, ax, bx, cx, dx);
            kterm_putf("\n - Features Tracked:");
            if (dx & (1 << 25))
                kterm_putf(" SSE");
            if (dx & (1 << 26))
                kterm_putf(" SSE2");
            if (cx & (1 << 0))
                kterm_putf(" SSE3");
            if (cx & (1 << 9))
                kterm_putf(" SSSE3");
            if (cx & (1 << 19))
                kterm_putf(" SSE4.1");
            if (cx & (1 << 20))
                kterm_putf(" SSE4.2");
            if (cx & (1 << 28))
                kterm_putf(" AVX");

            kterm_putf("\n - Total APs Running: %d", kacpi_apsrunning + 1);
        }
        else if (kcmd_strequal(kterm_argv[1], "mem"))
        {
            kmem_printinfo();
        }
        else if (kcmd_strequal(kterm_argv[1], "pci"))
        {
            kterm_putf("\nkpci_info: current pci device table");
            kpci_device* kpci_table = k_infotable.kpci_table;
            for (int i = 0; i < k_infotable.kpci_tablesize; i++)
            {
                kterm_putf("\n - %2x:%2x:%2x:%2x ", kpci_table[i].section, kpci_table[i].bus, kpci_table[i].device, kpci_table[i].function);
                kterm_putf("VendorID %4x DeviceID %4x [%s]/[%s]",
                    kpci_getvendorid(&kpci_table[i]), kpci_getdeviceid(&kpci_table[i]),
                    kpci_getclassname(kpci_getbaseclass(&kpci_table[i])),
                    kpci_getsubclassname(kpci_getbaseclass(&kpci_table[i]), kpci_getsubclass(&kpci_table[i])));
            }
        }
        else
            kterm_putf("\nInvalid subcommand.");
    }
    else if (kterm_argc == 1)
    {
        kterm_putf("\n%s", kterm_infotext);
        kterm_putf("\nAvailable subcommands: cpu, mem, pci");
    }
    else
        kterm_putf("\nInvalid argument count.");
}

void kcmd_readfile(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc < 2)
    {
        kterm_putf("\nNot enough arguments.");
        return;
    }

    if (kterm_getpartition() == -1)
    {
        kterm_putf("\nInvalid partition selection.");
        return;
    }

    kfs_partition *selected_partition = k_infotable.kfs_partitions[kterm_getpartition()];

    if ((uint64_t)selected_partition == 0)
    {
        kterm_putf("\nPartition does not exist.");
        kterm_setpartition(-1);
        return;
    }
    
    char *filename = 0;
    if (kterm_argv[1])
        filename = kterm_argv[1];

    size_t file_length;
    uint8_t *file = 0;

    char *fullname = kterm_getabsolutedir(filename);
    
    file = kfs_readfile(selected_partition, fullname, &file_length);

    kmem_kfree(fullname);

    if ((uint64_t)file)
    {
        kterm_putf("\nhex output 0x%x bytes:\n", file_length);
        int i;
        for (i = 0; i < (file_length - 7); i += 8)
            kterm_putf("%2x%2x%2x%2x%2x%2x%2x%2x", 
                file[i], file[i + 1], file[i + 2], file[i + 3], 
                file[i + 4], file[i + 5], file[i + 6], file[i + 7]);
        for (; i < file_length; i++)
            kterm_putf("%2x", file[i]);
        kmem_free(file, (file_length + 0x1000 - 1) / 0x1000);
    }
    else
        kterm_putf("\nFile not found!");
}

void kcmd_shutdown()
{
    kterm_putf("\nTrying to shutdown from ACPI...");
    kacpi_shutdown();
}

void kcmd_test(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc == 2)
    {
        if (kcmd_strequal(kterm_argv[1], "mouse"))
        {
            kmouse_test();
            kkeyboard_setinput(*kterm_input);
        }
        else
            kterm_putf("\nInvalid subcommand.");
    }
    else if (kterm_argc == 1)
    {
        kterm_putf("\nAvailable subcommands: mouse");
        uint16_t* test = kmem_palloc(2, kmem_paging_1kb);
        uint16_t* test2 = kmem_page((uint64_t)&test[15], 0x1000, kmem_paging_present | kmem_paging_writable, kmem_paging_1kb);
        kterm_putf("\ntest %x", (uint64_t)test2);
        test2[32] = 0xCA;
        kterm_putf("\ntest2[32] %x", test2[32]);
        kmem_unpage(test2, 0x1000);
        kmem_pfree(test, 2);
        uint16_t* test3 = kmem_kalloc(1024);
        kterm_putf("\ntest3 %x", test3);
        kmem_kfree(test3);
        kterm_putf("\nwait a few second :) -");
        for (uint16_t i = 1; i <= 5; i++)
        {
            ksleep(1000);
            kterm_putf(" %d", i);
        }
    }
    else
        kterm_putf("\nInvalid argument count.");
}

void kcmd_wait(char *kterm_argv[], int kterm_argc)
{
    if (kterm_argc < 2)
        kterm_putf("\nNot enough arguments.");
    else if (kterm_argc == 2)
    {
        int64_t input = 0;
        if (kterm_argv[1])
            input = str_atoi(kterm_argv[1]);
        if (input >= 0)
        {    
            kterm_putf("\nWaiting %d seconds...", input);
            ksleep(input * 1000);
        }
        else
            kterm_putf("\nInvalid number.");
    }
    else
        kterm_putf("\nInvalid argument count.");
}

void kcmd_runcommand(char *kterm_argv[], int kterm_argc)
{
    if (kcmd_strequal(kterm_argv[0], "cd"))
        kcmd_cd(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "clear"))
        kcmd_clear();
    else if (kcmd_strequal(kterm_argv[0], "color"))
        kcmd_color(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "compare"))
        kcmd_compare(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "crash"))
        kcmd_crash();
    else if (kcmd_strequal(kterm_argv[0], "dir"))
        kcmd_dir(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "fs"))
        kcmd_fs(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "font"))
        kcmd_font();
    else if (kcmd_strequal(kterm_argv[0], "help"))
        kcmd_help();
    else if (kcmd_strequal(kterm_argv[0], "image"))
        kcmd_image(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "info"))
        kcmd_info(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "read_file"))
        kcmd_readfile(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "shutdown"))
        kcmd_shutdown();
    else if (kcmd_strequal(kterm_argv[0], "test"))
        kcmd_test(kterm_argv, kterm_argc);
    else if (kcmd_strequal(kterm_argv[0], "wait"))
        kcmd_wait(kterm_argv, kterm_argc);
    else
        kterm_putf("\nCommand \'%s\' not found.\nUse the command \'help\' to list available commands.", kterm_argv[0]);
}