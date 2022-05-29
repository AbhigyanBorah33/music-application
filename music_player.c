/*******     to play audio, ./a.out wav_file_name < wav_file_name      ********/

#include <alsa/asoundlib.h>
#define PCM_DEVICE "default"
#define NUMBER_OF_MAX_SAMPLES 2606952

static int sample_rate;
static int bytes_per_sample;
static int num_samples;
int size_of_sample;
int total_file_size, data_size;
int format_type, num_channels, block_align;


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
    printf("[%s] Data size %d bytes, Number of samples %d, Size of each frame %d bytes\n", __FUNCTION__, data_size, num_samples, size_of_sample);
    
    fclose(ptr);

    return 1;
}

int main(int argc, char **argv)
{
    if(argc < 2){
		printf("No file argument found\n");
        return -1;
    }

	unsigned int pcm, tmp, dir;
	float seconds;
	snd_pcm_t *pcm_handle; 
	snd_pcm_hw_params_t *params; 
	snd_pcm_uframes_t frames;
	char *buff;
	int buff_size, loops;

    wiced_load_wave_file(argv[1]);
    seconds = (float)data_size/(sample_rate * size_of_sample);

	/* Open the PCM device in playback mode */
	if (pcm = snd_pcm_open(&pcm_handle, PCM_DEVICE,
					SND_PCM_STREAM_PLAYBACK, 0) < 0)
		printf("ERROR: Can't open \"%s\" PCM device. %s\n",
					PCM_DEVICE, snd_strerror(pcm));

	/* Allocate parameters object and fill it with default values*/
	snd_pcm_hw_params_alloca(&params); 

	snd_pcm_hw_params_any(pcm_handle, params); 

	/* Set parameters */
	if (pcm = snd_pcm_hw_params_set_access(pcm_handle, params,
					SND_PCM_ACCESS_RW_INTERLEAVED) < 0) 
		printf("ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_format(pcm_handle, params,
						SND_PCM_FORMAT_S16_LE) < 0) 
		printf("ERROR: Can't set format. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_channels(pcm_handle, params, num_channels) < 0) 
		printf("ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

	if (pcm = snd_pcm_hw_params_set_rate_near(pcm_handle, params, &sample_rate, 0) < 0) 
		printf("ERROR: Can't set rate. %s\n", snd_strerror(pcm));

    /* Write parameters */
	if (pcm = snd_pcm_hw_params(pcm_handle, params) < 0)
		printf("ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

	/* Resume information */
	printf("PCM name: '%s'\n", snd_pcm_name(pcm_handle));

	printf("PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(pcm_handle)));

	snd_pcm_hw_params_get_channels(params, &tmp);
	printf("Channels: %i ", tmp);

	if (tmp == 1)
		printf("(mono)\n");
	else if (tmp == 2)
		printf("(stereo)\n");

	snd_pcm_hw_params_get_rate(params, &tmp, 0);
	printf("Sample Rate: %d fps\n", tmp);

	printf("Duration: %f seconds\n", seconds);	

	/* Allocate buffer to hold single period */
	snd_pcm_hw_params_get_period_size(params, &frames, 0);

	buff_size = frames * num_channels * 2 /* 2 -> sample size */;
	buff = (char *) malloc(buff_size);

	snd_pcm_hw_params_get_period_time(params, &sample_rate, NULL);

	for (loops = (seconds * 1000000) / sample_rate; loops > 0; loops--) {
		if (pcm = read(0, buff, buff_size) == 0) {
			printf("Early end of file.\n");
			return 0;
		}

		if (pcm = snd_pcm_writei(pcm_handle, buff, frames) == -EPIPE) {
			printf("XRUN.\n");
			snd_pcm_prepare(pcm_handle);
		} else if (pcm < 0) {
			printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
		}
	}

	snd_pcm_drain(pcm_handle);
	snd_pcm_close(pcm_handle);
	free(buff);

	return 0;
}
