/* Calculates the following parameters of the wav file:
	Total File size in bytes, Format Type, Number of Channels,
	Sample rate, Block Align, Data size, Number of sample, Size
	of each sample

   To run: ./a.out <file name>
*/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define NUMBER_OF_MAX_SAMPLES 2606952
#define debug_Printf  printf

static int sample_rate;
static int bytes_per_sample;
static int num_samples;
static uint8_t  *pPCM = NULL;
static uint32_t    PCMSongSize = 0;

/* below two functions to find the file size */
int static wiced_file_read_helper(FILE **ptr, uint8_t num_bytes)
{
    int val = 0, index = 0;
    uint8_t buff[4];
    size_t result;

    if (num_bytes > 4)
        return 0;
    result = fread(buff, 1, num_bytes, *ptr);
    if (result != num_bytes)
        printf("[%s] Did not read %d bytes\n", __FUNCTION__, num_bytes);

    while (num_bytes)
    {
        val = val | (buff[index] << (8 * index));
        index++;
        num_bytes--;
    }
    return val;
}

uint8_t wiced_load_wave_file(char *file_name)
{
    FILE *ptr = fopen(file_name, "rb");

    int total_file_size;
    int format_type, num_channels, block_align, data_size;
    int size_of_sample;
    int index;
    size_t result;

    if (ptr == NULL)
    {
        printf("[%s] Error while opening %s wave file\n", __FUNCTION__, file_name);
        return 0;
    }
    // skip 4 byte "RIFF"
    fseek(ptr, 4, SEEK_SET);

    //read total file size (4 bytes)
    total_file_size = wiced_file_read_helper(&ptr, 4);
    printf("[%s] Total File size %d bytes\n", __FUNCTION__, total_file_size);

    //skip 4 byte "WAVE" marker, 4 byte fmt, 4 byte format len
    fseek(ptr, 12, SEEK_CUR);

    // 2 byte format. PCM = 1
    format_type = wiced_file_read_helper(&ptr, 2);
    printf("[%s] Format Type %d\n", __FUNCTION__, format_type);
    if (format_type != 1)
    {
        printf("[%s] Invalid Format type %d\n", __FUNCTION__, format_type);
        return 0;
    }

    // 2 byte number of channels
    num_channels = wiced_file_read_helper(&ptr, 2);
    printf("[%s] Number of Channels %d\n", __FUNCTION__, num_channels);
    if (num_channels != 2)
    {
        printf("[%s] Number of channel check failed %d\n", __FUNCTION__, num_channels);
        return 0;
    }

    // 4 byte sample rate
    sample_rate = wiced_file_read_helper(&ptr, 4);
    printf("[%s] Sample rate %d\n", __FUNCTION__, sample_rate);

    // skip 4 byte byterate
    fseek(ptr, 4, SEEK_CUR);

    // 2 byte block align
    block_align = wiced_file_read_helper(&ptr, 2);
    printf("[%s] Block Align %d\n", __FUNCTION__, block_align);

    // 2 bytes bits per sample
    bytes_per_sample = (wiced_file_read_helper(&ptr, 2) / 8);

    // skip 4 byte data string
    fseek(ptr, 4, SEEK_CUR);

    // read 4 bytes of data size
    data_size = wiced_file_read_helper(&ptr, 4);

    num_samples = (data_size) / (num_channels * bytes_per_sample);
    size_of_sample = num_channels * bytes_per_sample;
    printf("[%s] Data size %d, Number of sample %d, Size of each sample %d\n", __FUNCTION__, data_size, num_samples, size_of_sample);
    if (num_samples > NUMBER_OF_MAX_SAMPLES)
        num_samples = NUMBER_OF_MAX_SAMPLES;
        
        pPCM = (uint8_t *)malloc (data_size);
        PCMSongSize = fread (pPCM, 1, data_size, ptr);
        fclose (ptr);

    return 1;
}

void main (int argc, char *argv[])
{
    if (pPCM == NULL)
    {
        uint8_t file_done = wiced_load_wave_file(argv[1]);

        if (!file_done)
        {
            printf ("ERROR - Unable to open: %s!!!!",  argv[1]);
            return;
        }
    }
}
