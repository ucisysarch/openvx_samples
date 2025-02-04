#ifndef SIFT_COMMONS_H
#define SIFT_COMMONS_H

#include <stdio.h>
#include <stdlib.h>

#define OCTAVE_NUM 5
#define OCTAVE_LAYERS 5

#define MAX_KEYPOINTS_PER_THREE_DOGS 200

//valuables for custom convoution
static const vx_uint32 gaussian1Scale = 1024;
static const vx_uint32 gaussian2Scale = 1024;
static const vx_uint32 gaussian3Scale = 1024;
static const vx_uint32 gaussian4Scale = 8192;
//static const vx_uint32 gaussian5x5scale = 256;
//static const vx_uint32 gaussian7x7scale = 1024;
//static const vx_uint32 gaussian9x9scale = 65536;
//static const vx_uint32 gaussian11x11scale = 2097152;

//make convolution mask using pibonachi
static vx_int16 gaussian1[3][3] =
{
	{ 99, 120, 99 },
	{ 120, 148, 120 },
	{ 99, 120, 99 },
};

static vx_int16 gaussian2[3][3] =
{
	{ 111, 115, 111 },
	{ 115, 120, 115 },
	{111, 115, 111 },
};

static vx_int16 gaussian3[3][3] =
{
	{ 113, 114, 113 },
	{ 114, 116, 114 },
	{ 113, 114, 113 },
};

static vx_int16 gaussian4[3][3] =
{
	{ 909, 911, 909 },
	{ 911, 912, 911 },
	{ 909, 911, 909 },
};


/*
static vx_int16 gaussian5x5[5][5] =
{
	{ 1, 4, 6, 4, 1 },
	{ 4, 16, 24, 16, 4 },
	{ 6, 24, 36, 24, 6 },
	{ 4, 16, 24, 16, 4 },
	{ 1, 4, 6, 4, 1 }
};

static const vx_int16 gaussian7x7[7][7] =
{
	{ 1, 4, 6, 10, 6, 4, 1 },
	{ 4, 16, 24, 40, 24, 16, 4 },
	{ 6, 24, 36, 60, 36, 24, 6 },
	{ 10, 40, 60, 100, 60, 40, 10 },
	{ 6, 24, 36, 60, 36, 24, 6 },
	{ 4, 16, 24, 40, 24, 16, 4 },
	{ 1, 4, 6, 10, 6, 4, 1 },
};

static const vx_int16 gaussian9x9[9][9] =
{
	{ 1, 8, 28, 56, 70, 56, 28, 8, 1 },
	{ 8, 64, 244, 488, 560, 448, 224, 64, 8 },
	{ 28, 244, 784, 1568, 1960, 1568, 784, 224, 28 },
	{ 56, 448, 1568, 3136, 3920, 3136, 1568, 448, 56 },
	{ 70, 560, 1960, 3920, 4900, 3920, 1960, 560, 70 },
	{ 56, 448, 1568, 3136, 3920, 3136, 1568, 448, 56 },
	{ 28, 224, 784, 1568, 1960, 1568, 784, 224, 28 },
	{ 8, 64, 224, 448, 560, 448, 224, 64, 8 },
	{ 1, 8, 28, 56, 70, 56, 28, 8, 1 }
};
static const vx_int16 gaussian11x11[11][11] =
{
	{ 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1 },
	{ 10, 100, 450, 1200, 2100, 25200, 2100, 1200, 450, 100, 10 },
	{ 45, 450, 2025, 5400, 9450, 11340, 9450, 5400, 2025, 450, 45 },
	{ 120, 1200, 5400, 14400, 25200, 30240, 25200, 14400, 5400, 1200, 120 },
	{ 210, 2100, 9450, 25200, 44100, 52920, 44100, 25200, 9450, 2100, 210 },
	{ 252, 2520, 11340, 30240, 52920, 63504, 52920, 30240, 11340, 2520, 252 },
	{ 210, 2100, 9450, 25200, 44100, 52920, 44100, 25200, 9450, 2100, 210 },
	{ 120, 1200, 5400, 14400, 25200, 30240, 25200, 14400, 5400, 1200, 120 },
	{ 45, 450, 2025, 5400, 9450, 11340, 9450, 5400, 2025, 450, 45 },
	{ 10, 100, 450, 1200, 2100, 25200, 2100, 1200, 450, 100, 10 },
	{ 1, 10, 45, 120, 210, 252, 210, 120, 45, 10, 1 },
};
*/

//initialize custom convolvution.
static vx_convolution vxCreateGaussianConvolution(vx_context context, vx_int16 num)
{
	vx_status status;
	vx_convolution conv = NULL;

	switch (num)
	{
	case 1:
		conv = vxCreateConvolution(context, 3, 3);
		vxCopyConvolutionCoefficients(conv, (vx_int16*)gaussian1, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
		status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian1Scale, sizeof(vx_uint32));
		break;
	case 2:
		conv = vxCreateConvolution(context, 3, 3);
		vxCopyConvolutionCoefficients(conv, (vx_int16*)gaussian2, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
		status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian2Scale, sizeof(vx_uint32));
		break;
	case 3:
		conv = vxCreateConvolution(context, 3, 3);
		vxCopyConvolutionCoefficients(conv, (vx_int16*)gaussian3, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
		status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian3Scale, sizeof(vx_uint32));
		break;
	case 4: 
		conv = vxCreateConvolution(context, 3, 3);
		vxCopyConvolutionCoefficients(conv, (vx_int16*)gaussian4, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST);
		status = vxSetConvolutionAttribute(conv, VX_CONVOLUTION_SCALE, (void *)&gaussian4Scale, sizeof(vx_uint32));
		break;
	default:
		break;
	}

	if (status != VX_SUCCESS)
	{
		vxReleaseConvolution(&conv);
		return NULL;
	}
	return conv;
}

//Make 'imgname.png' from given vx_image.
void saveimage(char* imgname, vx_image* img)
{
	//write-only
	FILE* outf = fopen(imgname, "wb");

	//pgm file contains width, height value itself.
	vx_uint32 w, h;
	vxQueryImage((*img), VX_IMAGE_WIDTH, &w, sizeof(w));
	vxQueryImage((*img), VX_IMAGE_HEIGHT, &h, sizeof(h));
	printf("%s %d %d>>>>\n", imgname, w, h);

	//Set patch we are going to access from (0,0) to (width, height). This stands for entire image.
	vx_rectangle_t imrect;
	imrect.start_x = imrect.start_y = 0;
	imrect.end_x = w; imrect.end_y = h;
	vx_uint32 plane = 0;
	vx_imagepatch_addressing_t imaddr;
	void* imbaseptr = NULL;
	vx_map_id img_id;

	if (vxMapImagePatch((*img), &imrect, plane, &img_id, &imaddr, &imbaseptr, VX_READ_ONLY, VX_MEMORY_TYPE_NONE, VX_NOGAP_X) != VX_SUCCESS)
		printf("access failed\n");

	//write width, height, 255(max value for per pixel) to PGM file.
	fprintf(outf, "P5\n%d %d\n255\n", w, h);

	//write each pixel value as BINARY on file.
	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			vx_uint8 *ptr2 = (vx_uint8 *)vxFormatImagePatchAddress2d(imbaseptr, x, y, &imaddr);

			fprintf(outf, "%c", (*ptr2));
		}

	}


	
	vxUnmapImagePatch((*img), img_id);
	fclose(outf);
}

//record image's keypoints and descriptor
void recordImageStatus(vx_array* keypt_arr, vx_array* descr_arr, char* INPUT_IMAGE)
{

	//int na;
	//scanf("%d", &na);

	printf("recording status of image..\n");

	vx_size num_items[(OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2))];
	vx_size key_num_items[(OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2))];

	FILE* recordFile;
	char recordFilename[64] = { 0, };
	sprintf(recordFilename, "%s", INPUT_IMAGE);
	if ((recordFile = fopen(recordFilename, "w")) == NULL)
	{
		printf("can't write to recordFile..\n");
		exit(1);
	}

	//every status on one file
	for (int k = 0; k < (OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)); k++)
	{
		vxQueryArray(keypt_arr[k], VX_ARRAY_NUMITEMS, &num_items[k], sizeof(num_items[k]));
		vxQueryArray(descr_arr[k], VX_ARRAY_NUMITEMS, &key_num_items[k], sizeof(key_num_items[k]));

		printf("<keypoint array [%d]>\n", k);
		printf("found keypoints : %d\n", (int)num_items[k]);
		printf("<descriptor array [%d]\n", k);
		printf("descriptor values : %d (%d * 128 == %d)\n\n", key_num_items[k], num_items[k], (128 * num_items[k]));
		fprintf(recordFile, "<keypoint array [%d]>\n", k);
		fprintf(recordFile, "found keypoints : %d\n", num_items[k]);
		fprintf(recordFile, "<descriptor array [%d]\n", k);
		fprintf(recordFile, "descriptor values : %d (%d * 128 == %d)\n\n", key_num_items[k], num_items[k], (128 * num_items[k]));
	}

	for (int k = 0; k < (OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)); k++)
	{
		FILE* record_keypt = NULL;
		FILE* record_descr = NULL;

		char recordKeyptFilename[64] = { 0, };
		char recordDescrFilename[64] = { 0, };

		sprintf(recordKeyptFilename, "record\\%s_keypt%d.txt", INPUT_IMAGE, (k + 1));
		sprintf(recordDescrFilename, "record\\%s_descr%d.txt", INPUT_IMAGE, (k + 1));

		if ((record_keypt = fopen(recordKeyptFilename, "w")) == NULL)
			exit(1);
		if ((record_descr = fopen(recordDescrFilename, "w")) == NULL)
			exit(1);




		//keypoints
		fprintf(record_keypt, "%d\n", num_items[k]);
		if (num_items[k] > 0)
		{
			//access to keypoint array
			vx_size kpt_stride = 0ul;
			void* kpt_base = 0;
			vx_map_id kpt_arr_id;
			vxMapArrayRange(keypt_arr[k], (vx_size)0, (vx_size)num_items[k], &kpt_arr_id, &kpt_stride, (void**)&kpt_base,
				VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
			vx_int32 kpt_x, kpt_y;

			//write keypoint elements
			for (int i = 0; i < num_items[k]; i++)
			{
				vx_coordinates2d_t* xp = &vxArrayItem(vx_coordinates2d_t, kpt_base, i, kpt_stride);
				kpt_x = xp->x; kpt_y = xp->y;
				fprintf(record_keypt, "%d %d\n", kpt_x, kpt_y);
			}

			vxUnmapArrayRange(keypt_arr[k], kpt_arr_id);
		}

		//descriptor
		fprintf(record_descr, "%d\n", key_num_items[k]);
		if (key_num_items[k] > 0)
		{

			//access to descriptor array
			vx_size dsc_stride = 0ul;
			void* dsc_base = 0;
			vx_map_id dsc_arr_id;
			vxMapArrayRange(descr_arr[k], (vx_size)0, (vx_size)key_num_items[k], &dsc_arr_id, &dsc_stride, (void**)&dsc_base,
				VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);


			//write descriptor elements
			for (int i = 0; i < key_num_items[k]; i++)
			{
				if ((i != 0) && (i % 128 == 0)) fprintf(record_descr, "\n");

				vx_float32* xp = &vxArrayItem(vx_float32, dsc_base, i, dsc_stride);
				fprintf(record_descr, "%f ", (*xp));
			}

			vxUnmapArrayRange(descr_arr[k], dsc_arr_id);
		}

		fclose(record_keypt);
		fclose(record_descr);
	}

	//access each keypoint array and descriptor array write every elements

	fclose(recordFile);
	printf("recording complete.\n");

}
#endif
