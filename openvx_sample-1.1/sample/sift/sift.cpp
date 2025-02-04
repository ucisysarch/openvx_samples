#include <VX/vx.h>
#include <VX/vxu.h>
#include <stdio.h>
#include <stdlib.h>

#include "commons.h"

#define DEBUG_MODE true

/* Entry point. */
int main(int argc, char* argv[])
{
	//pgm file contains width, height value itself.
	int width;
	int height;
	
	if (argc != 2) {
	    printf("Usage: sift input_pgm_file\n");
	    exit(0);	
	}
    
	printf("opening %s.\n", argv[1]);
	FILE* in = fopen(argv[1], "rb");

	// Declare pointer of vx_uint8 as amount of width*height.
	vx_uint8* bytes;

	//Read width and height
	fscanf(in, "%*[^\n]\n%d %d\n%*[^\n]\n", &width, &height);

	bytes = (vx_uint8*)malloc(sizeof(vx_uint8)*width*height);

	//Fill bytes with pixel values
	for (int y = 0; y<height; y++)
		for (int x = 0; x<width; x++)
			fscanf(in, "%c", &bytes[(y*width) + x]);



	//Image reference for loading external image onto vximage
	vx_imagepatch_addressing_t addrs[] = {
		{
			width,
			height,
			sizeof(vx_uint8),
			width * sizeof(vx_uint8),
			VX_SCALE_UNITY,
			VX_SCALE_UNITY,
			1,
			1
		}
	};

	//wrap bytes as void*
	void* data[] = { bytes };


	//Create context.
	vx_context context = vxCreateContext();

	//finaly external image are set into vx_image. Now we can handle this as vx_image.
	vx_image image = vxCreateImageFromHandle(
		context,
		VX_DF_IMAGE_U8,
		addrs,
		data,
		VX_MEMORY_TYPE_HOST);


	//convolution values for gaussian blur
	vx_convolution conv1 = vxCreateGaussianConvolution(context, 1);
	vx_convolution conv2 = vxCreateGaussianConvolution(context, 2);
	vx_convolution conv3 = vxCreateGaussianConvolution(context, 3);
	vx_convolution conv4 = vxCreateGaussianConvolution(context, 4);

	//Create graph for context declared above
	vx_graph graph = vxCreateGraph(context);

	//=========================================================================================================
	//========================================== CREATING VIRTUAL IMAGES ======================================
	//=========================================================================================================

	//X gradient and Y gradient and magnitude of original images. Must be signed 16 bits images
	vx_image x_grad = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
	vx_image y_grad = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
	vx_image mag = vxCreateImage(context, width, height, VX_DF_IMAGE_S16);
	vx_image ori = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);	//phase function's output should be u8 type.


	//for test gradient image! soon will be eliminated
	//vx_image x_grad_test = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
	//vx_image y_grad_test = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
	//vx_image mag_test = vxCreateImage(context, width, height, VX_DF_IMAGE_U8);
	//vx_image ori_test = vxCreateVirtualImage(graph, 0, 0, VX_DF_IMAGE_U8);

	//Gaussian Pyramid
	vx_image gau_pyra[OCTAVE_NUM*OCTAVE_LAYERS];
	int nw = width;
	int nh = height;
	for (int i = 0; i < (OCTAVE_NUM); i++)
	{
		for (int j = 0; j < OCTAVE_LAYERS; j++)
			gau_pyra[i*OCTAVE_LAYERS + j] = vxCreateImage(context, nw, nh, VX_DF_IMAGE_U8);

		if (nw % 2 != 0) nw = (nw / 2) + 1;
		else nw /= 2;
		if (nh % 2 != 0) nh = (nh / 2) + 1;
		else nh /= 2;
	}


	//DOG Pyramid. Subtraction will cause negative output value so that we should prepare signed bit image.
	vx_image DOG_pyra[OCTAVE_NUM*(OCTAVE_LAYERS - 1)];
	nw = width; nh = height;
	for (int i = 0; i < (OCTAVE_NUM); i++)
	{
		for (int j = 0; j < OCTAVE_LAYERS - 1; j++)
			DOG_pyra[i*(OCTAVE_LAYERS - 1) + j] = vxCreateImage(context, nw, nh, VX_DF_IMAGE_U8);

		if (nw % 2 != 0) nw = (nw / 2) + 1;
		else nw /= 2;
		if (nh % 2 != 0) nh = (nh / 2) + 1;
		else nh /= 2;
	}


	//Create pyramid for basis of gaussian pyramid we're going to build. Only U8 is allowed.
	vx_pyramid pyra = vxCreateVirtualPyramid(graph, OCTAVE_NUM, VX_SCALE_PYRAMID_HALF, width, height, VX_DF_IMAGE_U8);

	//vx_scalar for converting depth (U8 -> S16)
	vx_int32 zero1 = 0;
	vx_int32 zero2 = 0;

	vx_scalar scalar1 = vxCreateScalar(context, VX_TYPE_INT32, (void *)&zero1);
	vx_scalar scalar2 = vxCreateScalar(context, VX_TYPE_INT32, (void *)&zero2);


	//several vx_array has itself responsiblity for one node function
	//containing keypoints
	vx_array keypt_arr[OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)];
	//vx_array verified_keypt_arr[OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)];
	//vx_image keypoint_img[OCTAVE_NUM*(OCTAVE_LAYERS - 1 - 2)];
	vx_array descrs[OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)];

	for (int i = 0; i < (OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)); i++)
	{
		keypt_arr[i] = vxCreateArray(context, VX_TYPE_COORDINATES2D, (MAX_KEYPOINTS_PER_THREE_DOGS));
		descrs[i] = vxCreateArray(context, VX_TYPE_FLOAT32, 30000);
	}


	//================================================================================================
	//========================================== CREATING NODES ======================================
	//================================================================================================


	if (vxSobel3x3Node(graph, image, x_grad, y_grad) == 0) printf("ERROR SOBEL NODE\n");

	if (vxMagnitudeNode(graph, x_grad, y_grad, mag) == 0) printf("ERROR MAGNITUDE NODE\n");

	// [!] phase function doesn't return radian value(0 ~ 2*PHI or 0 ~ 6.28). Rather they'll be mapped into u8 value(0 ~ 255)
	// < 0 ~ 6.28 => 0 ~ 255 >
	if (vxPhaseNode(graph, x_grad, y_grad, ori) == 0) printf("ERROR PHASE NODE\n");

	if (vxGaussianPyramidNode(graph, image, pyra) == 0) printf("ERROR GAUSSIANPYRAMID\n");

	//===== Building Gaussian pyramid =====//
	nw = width; nh = height;
	for (int i = 0; i < OCTAVE_NUM; i++)
	{
		//ex)	gau_pyra[0] (1st layer of octave 0) : original image
		//		gau_pyra[5] (1st layer of octave 1) : half sized of gau_pyra[0]
		//		gau_pyra[10] (1st layer of octave 2) : half sized of gau_pyra[5]
		//		gau_pyra[15] (1st layer of octave 3) : half sized of gau_pyra[10]
		//		gau_pyra[20] (1st layer of octave 4) : half sized of gau_pyra[15]
		if ((gau_pyra[i*OCTAVE_LAYERS] = vxGetPyramidLevel(pyra, i)) == 0) printf("WRONG INDEXING\n");


		//next layer is blurred image from previous one.
		//ex)	gau_pyra[0] = Original x1 size image		--octave 0
		//		gau_pyra[1] = BLUR( gau_pyra[0] )
		//		gau_pyra[2] = BLUR( gau_pyra[1] )
		//		gau_pyra[3] = BLUR( gau_pyra[2] )
		//		gau_pyra[4] = BLUR( gau_pyra[3] )
		//
		//		gau_pyra[5] = Original x0.5 size image		--octave 1
		//		gau_pyra[6] = BLUR( gau_pyra[5] )	
		//		gau_pyra[7] = BLUR( gau_pyra[6] )
		//		gau_pyra[8] = BLUR( gau_pyra[7] )
		//		gau_pyra[9] = BLUR( gau_pyra[8] )
		//
		for (int j = 1; j < OCTAVE_LAYERS; j++)
		{
			vx_image tempimage = vxCreateVirtualImage(graph, nw, nh, VX_DF_IMAGE_S16);

			if (j<2)
				vxConvolveNode(graph, gau_pyra[(i*OCTAVE_LAYERS) + (j - 1)], conv1, tempimage);
			else if (j<3)
				vxConvolveNode(graph, gau_pyra[(i*OCTAVE_LAYERS) + (j - 1)], conv2, tempimage);
			else if (j<4)
				vxConvolveNode(graph, gau_pyra[(i*OCTAVE_LAYERS) + (j - 1)], conv3, tempimage);
			else if (j<5)
				vxConvolveNode(graph, gau_pyra[(i*OCTAVE_LAYERS) + (j - 1)], conv4, tempimage);

			vxConvertDepthNode(graph, tempimage, gau_pyra[(i*OCTAVE_LAYERS) + j], VX_CONVERT_POLICY_WRAP, scalar1);
		}

		if (nw % 2 != 0) nw = (nw / 2) + 1;
		else nw /= 2;
		if (nh % 2 != 0) nh = (nh / 2) + 1;
		else nh /= 2;
	}


	//===== SUBTRACT TWO GAUSSIAN IMAGES WITH ABSDIFF TO BUILD DOG PYRAMID =====//
	for (int i = 0; i < OCTAVE_NUM; i++)
	{
		for (int j = 0; j < OCTAVE_LAYERS - 1; j++)
		{
			//DOG[i] = GAU[i] - GAU[i+1]

			if ((vxAbsDiffNode(graph, gau_pyra[(i*OCTAVE_LAYERS) + j], gau_pyra[(i*OCTAVE_LAYERS) + (j + 1)],
				DOG_pyra[(i*(OCTAVE_LAYERS - 1)) + j])) == 0) {
				printf("subtraction failed\n");
				return 0;
			}
		}
	}



	//to print out gradient images, soon will be eliminated

	//vxConvertDepthNode(graph, x_grad, x_grad_test, VX_CONVERT_POLICY_WRAP, scalar1);
	//vxConvertDepthNode(graph, y_grad, y_grad_test, VX_CONVERT_POLICY_WRAP, scalar1);
	//vxConvertDepthNode(graph, mag, mag_test, VX_CONVERT_POLICY_WRAP, scalar1);

	
	
	for (int i = 0; i < OCTAVE_NUM; i++)
	{
		for (int j = 0; j < (OCTAVE_LAYERS - 1 - 2); j++)
		{
			//printf("DOG [%d] [%d] [%d] (octave %d) => keypt_arr[%d]\n",
			//(i*(OCTAVE_LAYERS - 1)) + j, (i*(OCTAVE_LAYERS - 1)) + j + 1, (i*(OCTAVE_LAYERS - 1)) + j + 2, i,
			//(i*(OCTAVE_LAYERS - 1-2)) + j);

		
			//find keypoints from 3 DOG images
			if ((vxFindSiftKeypointNode(graph, mag, DOG_pyra[(i*(OCTAVE_LAYERS - 1)) + j], DOG_pyra[(i*(OCTAVE_LAYERS - 1)) + j + 1], DOG_pyra[(i*(OCTAVE_LAYERS - 1)) + j + 2], i,
				(vx_int32)(MAX_KEYPOINTS_PER_THREE_DOGS), keypt_arr[(i*(OCTAVE_LAYERS - 1 - 2) + j)])) == 0)
				printf("FINDSIFTKEYPOINT NODE FAILED\n");

			//make descriptor from verified keypoints above
			if (vxCalcSiftGradientNode(graph, ori, mag, keypt_arr[(i*(OCTAVE_LAYERS - 1 - 2) + j)], descrs[(i*(OCTAVE_LAYERS - 1 - 2) + j)]) == 0)
				printf("CALCSIFTGRADIENT NODE FAILED\n");

		}
	}
	
	

	// Running graph we created.
	vx_status final_status = vxVerifyGraph(graph);
	if (final_status == VX_SUCCESS)
	{
		printf("Graph verified!\n");
		final_status = vxProcessGraph(graph);
		if (final_status == VX_SUCCESS)
			printf("Graph processed.\n");
	}



	//recordImageStatus(keypt_arr, descrs);
	for (int k = 0; k < (OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)); k++) {
		vx_size array_len;
		vxQueryArray(keypt_arr[k], VX_ARRAY_NUMITEMS, &array_len, sizeof(vx_size));
		printf("found %d keypoints at DOG %d\n", (int)array_len, k);
	}


#ifdef DEBUG_MODE
	for (int k = 0; k < (OCTAVE_NUM * OCTAVE_LAYERS); k++) {
		char str[30];
		sprintf(str, "dog_file_%d.pgm", k);
		saveimage(str, &gau_pyra[k]);
	}
	
	vx_map_id map_id;
	vx_rectangle_t rect;
	rect.start_x = 0;
	rect.end_x = width;
	rect.start_y = 0;
	rect.end_y= height;
	char * mapped_data = 0;
	vx_imagepatch_addressing_t addressing = {
		width,
		height,
		sizeof(vx_uint8),
		width * sizeof(vx_uint8),
		VX_SCALE_UNITY,
		VX_SCALE_UNITY,
		1,
		1
	};

	vxMapImagePatch(image, &rect, 0, &map_id, &addressing, (void**)&mapped_data, VX_WRITE_ONLY, VX_MEMORY_TYPE_NONE, VX_NOGAP_X);
 	
	for (int k = 0; k < (OCTAVE_NUM * (OCTAVE_LAYERS - 1 - 2)); k++) {
		vx_size array_len;
		vxQueryArray(keypt_arr[k], VX_ARRAY_NUMITEMS, &array_len, sizeof(vx_size));
		if (array_len >0) {
			
			//access to keypoint array
			vx_size kpt_stride = 0ul;
			void* kpt_base = 0;
			vx_map_id kpt_arr_id;
			vxMapArrayRange(keypt_arr[k], (vx_size)0, (vx_size)array_len, &kpt_arr_id, &kpt_stride, (void**)&kpt_base,
				VX_READ_ONLY, VX_MEMORY_TYPE_HOST, 0);
			vx_int32 kpt_x, kpt_y;


			for (int l = 0; l < (int) array_len; l++)
			{
				vx_coordinates2d_t* xp = &vxArrayItem(vx_coordinates2d_t, kpt_base, l, kpt_stride);
				kpt_x = xp->x; kpt_y = xp->y;
				printf("%d %d\n", kpt_x, kpt_y);
				int index = (kpt_y)*width+kpt_x;
				if (index > 0) mapped_data[index-1] = 0x00;
				if (index < height*width-1) mapped_data[index+1] = 0x00;
				mapped_data[index] = 0x00;
				if (kpt_y > 0) mapped_data[(kpt_y-1)*width+kpt_x] = 0x00;
				if (kpt_y < height ) mapped_data[(kpt_y+1)*width+kpt_x] = 0x00;
				
			}

			vxUnmapArrayRange(keypt_arr[k], kpt_arr_id);		
		}

	}
	vxUnmapImagePatch(image, map_id);


	saveimage("keypoints.pgm", &image);
#endif

	//release data strutures created
	vxReleaseScalar(&scalar1);
	vxReleaseScalar(&scalar2);

	for (int i = 0; i < OCTAVE_LAYERS*OCTAVE_NUM; i++)
		vxReleaseImage(&gau_pyra[i]);

	for (int i = 0; i < (OCTAVE_LAYERS - 1 - 2)*OCTAVE_NUM; i++)
	{
		vxReleaseArray(&keypt_arr[i]);
		vxReleaseArray(&descrs[i]);
	}


	for (int i = 0; i < (OCTAVE_LAYERS - 1)*OCTAVE_NUM; i++)
	{
		vxReleaseImage(&DOG_pyra[i]);
	}

	vxReleaseImage(&x_grad);
	vxReleaseImage(&y_grad);
	vxReleaseImage(&mag);

	vxReleasePyramid(&pyra);
	vxReleaseGraph(&graph);
	vxReleaseContext(&context);

	int num;
	//scanf("%d", &num);
}
