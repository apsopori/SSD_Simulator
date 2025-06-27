// 202101001 - Ayush Chatta

/*

Note : 
make sure all the packages are installed in your system to run the code.
   
For Compilation:
    gcc 202101001_SSD_Simulation.c -o ssd

Example Command to run the code:
    ./ssd –T ideal –L w10:a -l 30 -B 3 -p 10
*/

// IE411: OS -> Project -> SSD Simulation

#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

#define TYPE_DIRECT 1
#define TYPE_LOGGING 2
#define TYPE_IDEAL 3

#define STATE_INVALID 1
#define STATE_ERASED 2
#define STATE_VALID 3

typedef struct
{
    int ssd_type;
    int num_logical_pages;
    int physical_write_sum;
    int num_blocks, pages_per_block, gc_current_block, physical_erase_sum, gc_high_water_mark, 
    gc_low_water_mark, current_page, current_block, gc_count, physical_read_sum, logical_trim_sum, logical_write_sum, logical_read_sum, logical_trim_fail_sum, 
    logical_write_fail_sum, logical_read_fail_sum;
    float block_erase_time, page_program_time, page_read_time;
    int *state, *forward_map, *reverse_map, *physical_erase_count, *physical_read_count, *physical_write_count, *gc_used_blocks;
    char *data;
} SSD_Struct;


SSD_Struct *initialize_ssd(int ssd_type, int num_logical_pages, int num_blocks, int pages_per_block, float block_erase_time, float page_program_time, float page_read_time, int high_water_mark, int low_water_mark)
{
    SSD_Struct *s = (SSD_Struct *)malloc(sizeof(SSD_Struct));
    s->ssd_type = ssd_type;
    s->num_logical_pages = num_logical_pages;
    s->num_blocks = num_blocks;
    s->pages_per_block = pages_per_block;
    s->block_erase_time = block_erase_time;
    s->page_program_time = page_program_time;
    s->page_read_time = page_read_time;

    int numberOfPages = num_blocks * pages_per_block;
    s->state = (int *)malloc(numberOfPages * sizeof(int));
    s->data = (char *)malloc(numberOfPages * sizeof(char));
    int itr;
    for (itr = 0; itr < numberOfPages;)
    {
        s->state[itr] = STATE_INVALID;
        s->data[itr] = ' ';
        itr+=1;
    }
    s->gc_used_blocks = (int *)malloc(num_blocks * sizeof(int));
    for (itr = 0; itr < num_blocks;)
    {
        s->gc_used_blocks[itr] = 0;
        itr+=1;
    }
    s->forward_map = (int *)malloc(num_logical_pages * sizeof(int));
    s->reverse_map = (int *)malloc(numberOfPages * sizeof(int));
    for (itr = 0; itr < num_logical_pages;)
    {
        s->forward_map[itr] = -1;
        itr+=1;
    }
    for (itr = 0; itr < numberOfPages;)
    {
        s->reverse_map[itr] = -1;
        itr+=1;
    }
    s->physical_erase_count=(int *)malloc(num_blocks*sizeof(int));
    s->physical_write_count=(int *)malloc(num_blocks*sizeof(int));
    s->physical_read_count=(int *)malloc(num_blocks*sizeof(int));
    for (itr=0;itr<num_blocks;)
    {
        s->physical_read_count[itr] = 0;
        s->physical_erase_count[itr] = 0;
        s->physical_write_count[itr] = 0;
        itr+=1;
    }
    s->logical_trim_sum = 0;
    s->logical_write_sum = 0;
    s->logical_read_sum = 0;
    s->physical_write_sum = 0;
    s->physical_erase_sum = 0;
    s->physical_read_sum = 0;
    s->logical_write_fail_sum = 0;
    s->logical_trim_fail_sum = 0;
    s->logical_read_fail_sum = 0;
    s->gc_count = 0;
    s->gc_current_block = 0;
    s->current_page = -1;
    s->current_block = 0;
    s->gc_high_water_mark = high_water_mark;
    s->gc_low_water_mark = low_water_mark;
    return s;
}


void physical_program(SSD_Struct *s, int address_of_page, char data)
{
    s->state[address_of_page] = STATE_VALID;
    s->data[address_of_page] = data;
    s->physical_write_count[address_of_page / s->pages_per_block]+=1;
    s->physical_write_sum++;
}


void physical_erase(SSD_Struct *s, int block_address)
{
    int page_begin = block_address * s->pages_per_block;
    int page_end = page_begin + s->pages_per_block - 1;

    for (int page = page_begin; page <= page_end; page++)
    {
        s->data[page] = ' ';
        s->state[page] = STATE_ERASED;
    }

    s->gc_used_blocks[block_address] = 0;
    s->physical_erase_count[block_address]++;
    s->physical_erase_sum++;
}


char physical_read(SSD_Struct *s, int address_of_page)
{
    s->physical_read_count[address_of_page / s->pages_per_block]+=1;
    s->physical_read_sum++;
    return s->data[address_of_page];
}


int is_block_free(SSD_Struct *s, int block_no)
{
    int initial_page = block_no * s->pages_per_block;
    if (s->state[initial_page] == STATE_ERASED || s->state[initial_page] == STATE_INVALID)
    {
        if (s->state[initial_page] == STATE_INVALID)
        {
            physical_erase(s, block_no);
        }
        s->current_page = initial_page;
        s->current_block = block_no;
        s->gc_used_blocks[block_no]=1;
        return 1;
    }
    return 0;
}


char *write_direct(SSD_Struct *s, int page_address, char data)
{
    int block_address = page_address / s->pages_per_block;
    int page_begin = block_address * s->pages_per_block;
    int page_end = page_begin + s->pages_per_block - 1;
    char old_data[s->pages_per_block];
    for (int itr = page_begin; itr <= page_end; itr++)
    {
        old_data[itr - page_begin] = (s->state[itr] == STATE_VALID) ? physical_read(s, itr) : ' ';
    }
    physical_erase(s, block_address);
    for (int itr = 0; itr < s->pages_per_block; itr++)
    {
        if (itr == page_address % s->pages_per_block || old_data[itr] == ' ')
            continue;
        physical_program(s, page_begin + itr, old_data[itr]);
    }
    physical_program(s, page_address, data);
    s->forward_map[page_address] = page_address;
    s->reverse_map[page_address] = page_address;
    return "Success";
}


void update_cursor(SSD_Struct *s)
{
    s->current_page+=1;
    if(s->current_page%s->pages_per_block==0)
    {
        s->current_page = -1;
    }
}


int get_cursor(SSD_Struct *s)
{
    if (s->current_page == -1)
    {
        int block_iterator;
        for (block_iterator=s->current_block;block_iterator<s->num_blocks;)
        {
            if (is_block_free(s, block_iterator))
            {
                return 0;
            }
            block_iterator+=1;
        }
        for (block_iterator = 0; block_iterator < s->current_block;)
        {
            if (is_block_free(s, block_iterator))
            {
                return 0;
            }
            block_iterator+=1;
        }
        return -1;
    }
    return 0;
}


void garbage_collect(SSD_Struct *s)
{
    int cleaned_blocks = 0;
    for (int block_iterator = s->gc_current_block; block_iterator < s->num_blocks; block_iterator++)
    {
        if (block_iterator == s->current_block)
            continue;
        int page_start = block_iterator*s->pages_per_block;
        if (s->state[page_start]==STATE_ERASED)
            continue;
        int live_pages[s->pages_per_block];
        int live_count = 0;
        for (int page = page_start; page < page_start + s->pages_per_block; page++)
        {
            int logical_page = s->reverse_map[page];
            if (logical_page!=-1 && s->forward_map[logical_page]==page)
            {
                live_pages[live_count] = page;
                live_count++;
            }
        }
        if (live_count==s->pages_per_block)
            continue;
        for (int itr = 0; itr < live_count;)
        {
            char data = physical_read(s, live_pages[itr]);
            int logical_page = s->reverse_map[live_pages[itr]];
            s->reverse_map[live_pages[itr]] = -1;
            write_direct(s, logical_page, data);
            itr+=1;
        }
        physical_erase(s, block_iterator);
        cleaned_blocks++;

        if (s->gc_high_water_mark>=cleaned_blocks)
        {
            s->gc_count++;
            s->gc_current_block = block_iterator;
            return;
        }
    }
}


void print_stats(SSD_Struct *s)
{
    printf("Physical Operations Per Block:\n");
    printf("Erases: ");
    for (int itr = 0; itr < s->num_blocks;)
    {
        printf("%d ", s->physical_erase_count[itr]);
        itr+=1;
    }
    printf("\nTotal Erases: %d\n", s->physical_erase_sum);
    printf("Writes: ");
    for (int itr=0;itr<s->num_blocks;)
    {
        printf("%d ", s->physical_write_count[itr]);
        itr+=1;
    }
    printf("\nTotal Writes: %d\n", s->physical_write_sum);
    printf("Reads: ");
    for (int itr=0; itr<s->num_blocks;)
    {
        printf("%d ", s->physical_read_count[itr]);
        itr+=1;
    }
    printf("\nTotal Reads: %d\n", s->physical_read_sum);
    printf("\nLogical Operations:\n");
    printf("Write count: %d (%d failed)\n", s->logical_write_sum, s->logical_write_fail_sum);
    printf("Read count: %d (%d failed)\n", s->logical_read_sum, s->logical_read_fail_sum);
    printf("Trim count: %d (%d failed)\n", s->logical_trim_sum, s->logical_trim_fail_sum);
    printf("\nTimes:\n");
    printf("Erase time: %.2f\n", s->physical_erase_sum * s->block_erase_time);
    printf("Write time: %.2f\n", s->physical_write_sum * s->page_program_time);
    printf("Read time: %.2f\n", s->physical_read_sum * s->page_read_time);
    printf("Total time: %.2f\n",s->physical_erase_sum*s->block_erase_time+s->physical_write_sum*s->page_program_time+s->physical_read_sum*s->page_read_time);
}


// void upkeep(SSD_Struct *s)
// {
//     if (s->gc_high_water_mark<=s->gc_low_water_mark)
//     {
//         garbage_collect(s);
//     }
// }
void upkeep(SSD_Struct *s)
{
    int free_blocks = 0;
    for (int i = 0; i < s->num_blocks; i++) {
        int pg = i * s->pages_per_block;
        if (s->state[pg] == STATE_ERASED || s->state[pg] == STATE_INVALID)
            free_blocks++;
    }

    if (free_blocks <= s->gc_low_water_mark) {
        garbage_collect(s);
    }
}


char *trim(SSD_Struct *s, int addr)
{
    s->logical_trim_sum+=1;
    if (addr<0 || addr>=s->num_logical_pages)
    {
        s->logical_trim_fail_sum+=1;
        return "fail: illegal trim address";
    }
    if (s->forward_map[addr] == -1)
    {
        s->logical_trim_fail_sum+=1;
        return "fail: uninitialized trim";
    }
    s->forward_map[addr]=-1;
    return "Success";
}


char printable_state(int state)
{
    if (state==STATE_ERASED)
    {
        return 'E';
    }
    else if (state==STATE_VALID)
    {
        return 'v';
    }
    else
    {
        return 'i';
    }
}


void dump(SSD_Struct *ssd) {
    int num_of_pages = ssd->num_blocks * ssd->pages_per_block;
    printf("FTL   ");
    int cnt = 0;
    int ftl_columns = (ssd->pages_per_block*ssd->num_blocks)/7;
    for (int itr=0; itr<ssd->num_logical_pages;) {
        if (ssd->forward_map[itr]==-1) 
        {
            itr+=1;
            continue;
        }
        cnt++;
        printf("%3d:%3d ", itr, ssd->forward_map[itr]);
        if (cnt > 0 && cnt % ftl_columns == 0) 
        {
            printf("\n      ");
        }
        itr+=1;
    }
    if (cnt == 0) {
        printf("(empty)");
    }
    printf("\n");

    printf("Block ");
    int max_block_len = snprintf(NULL, 0, "%d", ssd->num_blocks - 1);
    for (int itr = 0; itr < ssd->num_blocks;) {
        printf("%-*d ", max_block_len, itr);
        itr+=1;
    }
    printf("\n");

    int max_len=snprintf(NULL, 0,"%d", num_of_pages-1);
    for (int m = max_len; m > 0; m--) 
    {
        if (m == max_len) 
        {
            printf("Page  ");
        } 
        else 
        {
            printf("      ");
        }
        for (int itr = 0; itr < num_of_pages; itr++) {
            char out_str[max_len + 1];
            snprintf(out_str, sizeof(out_str), "%0*d", max_len, itr);
            printf("%c", out_str[max_len - m]);
            if ((itr + 1) % ssd->pages_per_block == 0) 
            {
                printf(" ");
            }
        }
        printf("\n");
    }

    printf("State ");
    for (int itr=0; itr < num_of_pages;) {
        printf("%c", printable_state(ssd->state[itr]));
        if ((itr+1) % ssd->pages_per_block==0) {
            printf(" "); // Space between blocks
        }
        itr+=1;
    }
    printf("\n");

    printf("Data  ");
    for (int itr = 0; itr < num_of_pages;) {
        if (ssd->state[itr] == STATE_VALID) 
        {
            printf("%c", ssd->data[itr]);
        } 
        else 
        {
            printf(" ");
        }
        if ((itr + 1) % ssd->pages_per_block == 0) {
            printf(" "); // Space between blocks
        }
        itr+=1;
    }
    printf("\n");

    printf("Live  ");
    for (int itr = 0; itr < num_of_pages;) {
        if (ssd->state[itr] == STATE_VALID && ssd->forward_map[ssd->reverse_map[itr]] == itr) 
        {
            printf("+");
        } 
        else 
        {
            printf(" ");
        }
        if ((itr + 1) % ssd->pages_per_block==0) {
            printf(" ");
        }
        itr+=1;
    }
    printf("\n");
}


char *write_ideal(SSD_Struct *s, int addr, char d) {
    if (addr<0 || addr>=s->num_logical_pages) {
        return "fail: illegal write address";
    }
    int page_address = addr;
    physical_program(s, page_address, d);
    s->reverse_map[page_address] = addr;
    s->forward_map[addr] = page_address;
    return "Success";
}


char *read_data(SSD_Struct *s, int addr)
{
    s->logical_read_sum+=1;
    if (addr<0 || addr>=s->num_logical_pages)
    {
        s->logical_read_fail_sum+=1;
        return "fail: illegal read address";
    }
    if (s->forward_map[addr]==-1)
    {
        s->logical_read_fail_sum+=1;
        return "fail: uninitialized read";
    }
    char *data = malloc(2);
    data[0] = physical_read(s, s->forward_map[addr]);
    data[1] = '\0';  
    printf("Forward map for logical %d: %d\n", addr, s->forward_map[addr]);
    return data;
}


char *write_data(SSD_Struct *s, int addr, char d)
{
    s->logical_write_sum+=1;
    if (addr<0 || addr>=s->num_logical_pages)
    {
        s->logical_write_fail_sum++;
        return "Fail: Illegal write address";
    }
    char *result = NULL;
    printf("$$\n");
    if (s->ssd_type==TYPE_DIRECT)
    {
        printf("$$\n");
        result = write_direct(s, addr, d);
    }
    else if (s->ssd_type==TYPE_IDEAL) 
    {
        result = write_ideal(s, addr, d);
    } 
    else 
    {
        result = write_logging(s, addr, d);
    }
    if (result == NULL)
    {
        return "Fail: Write operation failed";
    }
    return result;
}


char *write_logging(SSD_Struct *s, int addr, char d) {
    if (addr<0 || addr>=s->num_logical_pages) {
        return "fail: illegal write address";
    }
    if (get_cursor(s) == -1) {
        return "fail: no free blocks available";
    }
    int page_address = s->current_page;
    physical_program(s, page_address, d);
    s->forward_map[addr] = page_address;
    s->reverse_map[page_address] = addr;
    update_cursor(s);
    printf("Mapping logical %d -> physical %d\n", addr, page_address);
    return "Success";
}


int main(int argc, char *argv[])
{
    int num_blocks = 7, low_water_mark = 8, pages_per_block = 10, high_water_mark = 10, num_logical_pages = 50;
    float block_erase_time = 1000.0, page_program_time = 40.0, page_read_time = 10.0;
    int ssd_type = TYPE_DIRECT;
    bool show_state = false, show_stats = false;

    int opt;
    while ((opt = getopt(argc, argv, "T:l:B:p:G:g:R:W:E:Ss"))!=-1)
    {
        switch (opt)
        {
        case 'T':
            if (strcmp(optarg, "direct")==0)
            {
                ssd_type = TYPE_DIRECT;
            }
            else if (strcmp(optarg, "log")==0)
            {
                ssd_type = TYPE_LOGGING;
            }
            else if (strcmp(optarg, "ideal")==0)
            {
                ssd_type = TYPE_IDEAL;
            }
            else
            {
                fprintf(stderr, "Invalid SSD type: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
            break;
        case 'l':
            num_logical_pages=atoi(optarg);
            break;
        case 'B':
            num_blocks=atoi(optarg);
            break;
        case 'p':
            pages_per_block=atoi(optarg);
            break;
        case 'G':
            high_water_mark=atoi(optarg);
            break;
        case 'g':
            low_water_mark=atoi(optarg);
            break;
        case 'R':
            page_read_time=atof(optarg);
            break;
        case 'W':
            page_program_time=atof(optarg);
            break;
        case 'E':
            block_erase_time=atof(optarg);
            break;
        case 'S':
            show_state=true;
            break;
        case 's':
            show_stats=true;
            break;
        default:
            fprintf(stderr, "Usage: %s [-T type] [-l logical_pages] [-B blocks] [-p pages/block]\n""           [-G high_water] [-g low_water] [-R read_time]\n""           [-W write_time] [-E erase_time] [-S (show state)]\n""           [-s (show stats)]\n",argv[0]);
            exit(EXIT_FAILURE);
        }
    }
    SSD_Struct *ssd = initialize_ssd(ssd_type, num_logical_pages, num_blocks, pages_per_block, block_erase_time, page_program_time, page_read_time, high_water_mark, low_water_mark);
    printf("Write to logical page 5: %s\n", write_data(ssd, 5, 'A'));
    if(ssd_type==TYPE_LOGGING)
    {
        printf("Read from logical page 5: %s\n", read_data(ssd,5));
    }
    else
    {
        printf("Read from logical page 5: %s\n", read_data(ssd, ssd->forward_map[5]));
    }
    upkeep(ssd);
    if (show_state)
        dump(ssd);
    if (show_stats)
        print_stats(ssd);
    free(ssd->state);
    free(ssd->data);
    free(ssd->forward_map);
    free(ssd->reverse_map);
    free(ssd->physical_erase_count);
    free(ssd->physical_read_count);
    free(ssd->physical_write_count);
    free(ssd->gc_used_blocks);
    free(ssd);
    return 0;
}

/*
    This command shows the direct type SSD simulation
    ./ssd -T direct -l 50 -B 10 -p 3 -S -s
*/