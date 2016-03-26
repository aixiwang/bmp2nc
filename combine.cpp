//-------------------------------------------------
// Copyright by Aixi Wang (aixi.wang@hotmail.com)
//
//-------------------------------------------------


#include "stdafx.h"
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

#include "typedef.h"


#define DEBUGSTR  0

#define	BIOS_FILE_SIZE 0x100000
#define EC_FILE_SIZE	0x10000
#define EC_FILL_BYTE	0x00

char  str1[256];
char  str2[256];

dword bmp_x_length, bmp_y_length;
dword x_phisical_length = 20000;
dword y_phisical_length = 20000;

dword bmp_data_bytes;
dword bmp_file_bytes;
dword bmp_bytes_per_x;
byte* p_bmp_data;

byte cnc_param1 = 1;
byte cnc_param2 = 1;
byte cnc_param3 = 1;
byte cnc_param4 = 0;


dword cnc_steps = 1;

long z_depth = -1000; //   
long z_travel_height = 1000;
long drill_mm_pmin = 250;
long travel_mm_pmin = 500;

extern dword x_steps_per_mm;
extern dword y_steps_per_mm;
extern dword z_steps_per_mm;


long cur_x;
long cur_y;
long cur_z;

long new_x=0;
long new_y=0;
long new_z=0;

void	print_256bs_hex (byte * p_byte_buffer, dword virtual_address);
int		dump_ram_to_file(char * str_filename, byte * addr, dword bytes);
int		read_file_to_ram(char * str_filename, byte * addr, dword bytes);

byte	LoadMonoBmp(char* filename);
byte	find_new_black_pt(long* px1, long* py1);
byte	find_next_black_pt(long x1, long  y1, long* px2, long* py2);
void	process_bmp_data(void);
void	process_bmp_data1(void); // used in v110
void	process_bmp_data2(void); // used in v110
void	process_bmp_data3(void); // used in v111
void	process_bmp_data4(void); // used in v116


void	goto_abs_axis_um(long abs_x, long abs_y, long abs_z);


byte*	pt_array1;

void	fill_array_data(void);
void	set_xy(byte* p_array, dword x, dword y, byte val);

byte	get_xy(byte* p_array, dword x, dword y);
dword	GetXYSqure(dword x1, dword y1, dword x2, dword y2);


//byte bios_buffer[BIOS_FILE_SIZE];

dword	min_bmp_x, max_bmp_x;
dword	min_bmp_y, max_bmp_y;

void search_min_max_x(void);
void search_min_max_y(void);
byte isBlackPoint(dword x, dword y);
void ClearBlackPoint(dword x, dword y);

//==========================================================================================
int main(int argc, char* argv[])
{

//	    char  str3[256];

//        dword i,j;

        if (argc >= 10)
        {  
			
			strcpy(str1, argv[1]);
           	strcpy(str2, argv[2]);
			sscanf(str2,"%d",&x_phisical_length);

           	strcpy(str2, argv[3]);
			sscanf(str2,"%d",&y_phisical_length);

           	strcpy(str2, argv[4]);
			sscanf(str2,"%d",&z_travel_height);

			strcpy(str2, argv[5]);
			sscanf(str2,"%d",&z_depth);

           	//strcpy(str3, argv[3]);

			strcpy(str2, argv[6]);
			sscanf(str2,"%d",&cnc_param1);

			strcpy(str2, argv[7]);
			sscanf(str2,"%d",&cnc_param2);

			strcpy(str2, argv[8]);
			sscanf(str2,"%d",&cnc_param3);

			strcpy(str2, argv[9]);
			sscanf(str2,"%d",&cnc_param4);
			

		}
        
		else
        {

		printf("BMP2NC.EXE V1.21 -  Copyright(c) 2009 Aixi.Wang\n");
		printf("Usage: BMP2NC bmp_filename x_phisical_length y_phisical_length z_travel_height z_depth cnc_param1 cnc_param2 cnc_param3 mode\n");
		printf("example: BMP2NC cnc.bmp 20000 20000 1000 -1000 1 1 1 0 -- generate path through shortest length\n");
		printf("         BMP2NC cnc.bmp 20000 20000 1000 -1000 1 1 1 1 -- generate path through bitmap method\n");

		return 1;
		}



	//strcat(str1, "dms.bmp");

	if (LoadMonoBmp(str1) == 0)
	{	
		printf("load %s fail!\n", str1);
		return 1;
	}
	//else
	//printf("load cnc.bmp ok!\n");



	//process_bmp_data(byte* bmp_data,dword x, dword y);
	p_bmp_data  = (byte*)(malloc(bmp_file_bytes));
	if (read_file_to_ram(str1, p_bmp_data, bmp_file_bytes))
	{
		//printf("read bmp data ok!\n");
		p_bmp_data += 0x3e;
	}
	else
	{
			//free(p_bmp_data);
		printf("read cnc.bmp data fail!\n");
		return 1;
	}

	//printf("\n============== STEP2 ===============\n");	




	printf("N1 G90\n");
	printf("N2 G0 X0 Y0 Z5\n");
	//printf("N3 G1\n");
	cnc_steps = 3;

		pt_array1  = (byte*)(malloc(bmp_x_length * bmp_y_length));
		fill_array_data();
		search_min_max_x();
		search_min_max_y();

	if (cnc_param4)
	{
		process_bmp_data4();	// bitmap
	}
	else
	{

		process_bmp_data3();	// shortest length
	}
	//printf("M00\n");
	printf("N%d G0 X0 Y0 Z5\n", cnc_steps);
	printf("M%d M30\n", cnc_steps+1);	


		return 0;
}






//----------------
// search_min_max_x
//----------------

void
search_min_max_x(void){
	min_bmp_x = bmp_x_length-1;
	max_bmp_x = 0;


	dword i,j;


	for(i=0; i<bmp_y_length; i++)		//  y
		for(j=0; j<bmp_x_length; j++)	//	x
		{
			if (pt_array1[i*bmp_x_length + j])
			{
				if (j < min_bmp_x)
					min_bmp_x = j;

				if (j > max_bmp_x)
					max_bmp_x = j;

			}
		
		}

	
//	printf("min_bmp_x = %d, max_bmp_x = %d\n", min_bmp_x, max_bmp_x);


}
//----------------
// search_min_max_y
//----------------

void
search_min_max_y(void){
	min_bmp_y = bmp_y_length-1;
	max_bmp_y = 0;


	dword i,j;


	for(i=0; i<bmp_y_length; i++)		//  y
		for(j=0; j<bmp_x_length; j++)	//	x
		{
			if (pt_array1[i*bmp_x_length + j])
			{
				if (i < min_bmp_y)
					min_bmp_y = i;

				if (i > max_bmp_y)
					max_bmp_y = i;

			}
		
		}

	
//	printf("min_bmp_x = %d, max_bmp_x = %d\n", min_bmp_x, max_bmp_x);


}
/*---------------------;
; dump_ram_to_file     ;
;----------------------;------------------------------------
; input :   1. byte buffer ptr
;           2. virtual_address
; output:   1: sucess
;           0: fail
; Description: none
;
; notes :
;-----------------------------------------------------------*/
int
dump_ram_to_file(char * str_filename, byte * addr, dword bytes)
{
   FILE    * fp;
   dword   i;
   byte    c;

   if ( !( fp=fopen(str_filename, "wb")) )
      {  printf("Open file %s error. \n", str_filename);
         return 0; };

        fseek(fp, 0, SEEK_SET);
        for (i = 0; i< bytes; i++)
        {  
			  c = *(addr+i);
              fwrite(&c, 1, 1, fp); 
		}

        fclose(fp);

        return 1;

}

/*---------------------;
; read_file_to_ram     ;
;----------------------;------------------------------------
; input :   1. byte buffer ptr
;           2. virtual_address
; output:   1: sucess
;           0: fail
; Description: none
;
; notes :
;-----------------------------------------------------------*/
int
read_file_to_ram(char * str_filename, byte * addr, dword bytes)
{
   FILE    * fp;
   dword   i;
   byte    c;

   if ( !( fp=fopen(str_filename, "rb")) )
      {  printf("Open file %s error. \n", str_filename);
         return 0; };

        fseek(fp, 0, SEEK_SET);
        for (i = 0; i< bytes; i++)
        {  
			  if (feof(fp) )
				  return 1;
			
              fread(&c, 1, 1, fp); 
			  *(addr+i) = c;
			  


		}

        fclose(fp);

        return 1;

}


/*---------------------;
; print_256bs_hex      ;
;----------------------;------------------------------------
; input :   1. byte buffer ptr
;           2. virtual_address
; output:   none
;
; Description: none
;
; notes :
;-----------------------------------------------------------*/
void
print_256bs_hex (byte * p_byte_buffer, dword virtual_address)
{
        int     i,j;
	byte    c,d;
       printf("\nPhysical Base Address: %lx\n", virtual_address);
        for (i = 0; i < 16; i++)
        {
           for (j = 0; j < 16; j++)
           {   c = p_byte_buffer[i * 16 + j];

	       d = c & 0x0f;

	       printf("%1x",c>>4);
	       printf("%1x",d );
	       putchar(' '); }
	   printf("   ");
	   for (j = 0; j < 16; j++)
           {   c = p_byte_buffer[i * 16 + j];
	       if ( c !=0x0a && c != 0x0d && c != 0x07 && c!=0x0c && c!=0x09)
	       printf("%c",c); }

	   putchar('\n'); }

}




//-----------------------------------------------------------
// bmp process
//-----------------------------------------------------------




byte
LoadMonoBmp(char* filename){
byte btemp[0x3e];

	
	//printf("read %s file ...\n", filename);

	if (read_file_to_ram(filename, btemp, 0x3e))
	{

		//printf("error 1");

		if ( (btemp[0] != 'B') || (btemp[1] != 'M') )
			return 0;

		bmp_file_bytes = *(dword*)(btemp+0x02);
		bmp_data_bytes = *(dword*)(btemp+0x22);
		bmp_x_length = *(dword*)(btemp+0x12);
        bmp_y_length = *(dword*)(btemp+0x16);
		
		//x_phisical_length = bmp_x_length/10;

		bmp_bytes_per_x = ((bmp_x_length + 31)/32)*4;

		//printf("bmp_file_bytes = %d\n", bmp_file_bytes);
		//printf("bmp_data_bytes = %d\n", bmp_data_bytes);
		//printf("bmp_x_length = %d\n", bmp_x_length);
		//printf("bmp_y_length = %d\n", bmp_y_length);
		//printf("x_phisical_length = %d\n", x_phisical_length);

		//printf("bmp_bytes_per_x = %d\n", bmp_bytes_per_x);

		if ( (bmp_bytes_per_x * bmp_y_length) != bmp_data_bytes )
		{
			printf("it's not a valid mono color bmp file!\n");
			return 0;
		}
		//else
		//	printf("it's a valid mono color bmp file!\n");

		// continue data process
	
		// copy data to pt_array1, pt_array2


		

		
		return 1;

	}
	else
	{
		return 0;
	}


}

//---------------------------
// fill_array_data
//---------------------------
void	
fill_array_data(void){
	int i,j;
	dword d1 = 0;

	for(i=0; i<bmp_y_length; i = i+ 1)		//  y
	{
	//	printf("(==y line %d=============================================)\n", i);

		for(j=0; j<bmp_x_length; j = j+1)	//	x
		{
			pt_array1[i*bmp_x_length + j] = isBlackPoint(j, i);
	//		printf("(%d, %d = %d)\n", j, i, (pt_array1[i*bmp_x_length + j]));	// debug
				
			if (pt_array1[i*bmp_x_length + j])
				d1++;
		}

	}
#if DEBUGSTR
	printf("total d1 = %d\n", d1);
#endif


};

//-------------
// GetXYSqure
//-------------


dword 
GetXYSqure(dword x1, dword y1, dword x2, dword y2){
	
	dword sq1, sq2;

	if (x1 > x2)
		sq1 = (x1-x2)*(x1-x2);
	else
		sq1 = (x2-x1)*(x2-x1);

	if (y1 > y2)
		sq2 = (y1-y2)*(y1-y2);
	else
		sq2 = (y2-y1)*(y2-y1);

	return ( sq1 + sq2);
}

//---------------------------
// get_xy
//---------------------------
byte
get_xy(byte* p_array, dword x, dword y){

	//printf("get_xy x=%d, y=%d, val = %d\n", x, y, p_array[y*bmp_x_length + x]); 
	return p_array[y*bmp_x_length + x];


}





//---------------------------
// set_xy
//---------------------------

void
set_xy(byte* p_array, dword x, dword y, byte val){
	

	p_array[y*bmp_x_length + x] = val;
	
#if 0
	if ((x ==0) && (y==0))
	{
		printf(" clear %d, %d, cur_val =%d \n", x, y, p_array[y*bmp_x_length + x]);
		

	}
#endif

}



//----------------------------
// return:  0  no 
//          1  found, no need let z high, then low
//          2  found, need z high, hen low
//
//----------------------------
#define HIGH_Z_LENGTH_LIMIT	cnc_param3
#define FIRST_PT_X bmp_x_length/2 
#define	FIRST_PT_Y bmp_y_length/2
#define PT_X_LENGTH	bmp_x_length
#define PT_Y_LENGTH	bmp_y_length


dword total_detected_points = 0;

byte
find_next_nearest_pt(dword last_x1, dword last_y1, dword* new_x1, dword* new_y1){

	dword shortest_length = 0xffffffff;
	dword temp_length;
	byte shorest_found = 0;
	dword shortest_x1 = 0, shortest_y1 = 0;

	
	dword i,j;

	shorest_found = 0;

//	printf("find_next x1=%d, y1=%d, \n", last_x1, last_y1);


	for(i=0; i<PT_Y_LENGTH; i += cnc_param2)
		for(j=0; j<PT_X_LENGTH; j += cnc_param1)
		{
			// if (j,i) == 1,  caculate (j,i) <---> (last_x1, last_y1)
//byte	isBlockBlackPoint(dword x1, dword y1);
//void ClearBlackPoint(dword x, dword y);

			//if (get_xy(pt_array1, j,i))
			if (isBlackPoint(j,i))
			{
				temp_length = GetXYSqure(j, i, last_x1, last_y1);

				if (temp_length < shortest_length)
				{
					shortest_length = temp_length;
					shortest_x1 = j;
					shortest_y1 = i;

					shorest_found = 1;

#if DEBUGSTR
					printf("( found shotest length = %d, x1 =%d, y1 = %d, x2 = %d, y2 = %d)\n",shortest_length, j,i, last_x1, last_y1);					
#endif


				}


			}

		}


	if (shorest_found ==0)
	{
	//	puts("can't found \n");
#if DEBUGSTR
		printf("total_detected_points = %d\n", 	total_detected_points);
#endif
		return 0;
	}

	*new_x1 = shortest_x1;
	*new_y1 = shortest_y1;
	//set_xy(pt_array1, shortest_x1, shortest_y1, 0);
	ClearBlackPoint(shortest_x1, shortest_y1);

#if DEBUGSTR
//	printf("(new_x1 =%d, new_y1=%d            )\n", *new_x1, *new_y1);

//	return 2;
#endif


	total_detected_points++;

	
	
	if (shortest_length > HIGH_Z_LENGTH_LIMIT)
		return 2;
	else
		return 1;




}

long
convert_2_cur_x(dword x1){

#if DEBUGSTR

#endif


#if 1
	float f1,f2;
	x1 = x1 - min_bmp_x;

	f1 = x_phisical_length;
	f1 = f1/(max_bmp_x - min_bmp_x);
	f2 = f1;
	f1 = x1*f1;

//	printf("(x f1=%f, long(f1)=%d, delta=%f, %f, %d, %d)\n", f1, (long)f1,f1 - (long)f1, f2, (long)f2, x_phisical_length*x1 );
	if ((f1 - (long)f1) >= 0.5)
	{
		if ((long)f1 != cur_x)
			f1++;
	}


	return (long)f1;


#else
//		x1 = x1 - min_bmp_x;
//	return ((x1 - min_bmp_x)*x_phisical_length/(max_bmp_x-min_bmp_x));
//	return (x1-min_bmp_x);

	return (x1);


#endif


}

long
convert_2_cur_y(dword y1){

#if 1

	float f1,f2;

	y1 = y1 - min_bmp_y;
	f1 = y_phisical_length;
	f1 = f1/(max_bmp_y - min_bmp_y);
	f2 = f1;
	f1 = y1*f1;

	
	if ((f1 - (long)f1) >= 0.5)
	{

		if ((long)f1 != cur_y)
			f1++;
		//f1++;
	}
	return (long)f1;

#else

//	return (y1*x_phisical_length/(max_bmp_x-min_bmp_x));
	return y1;
#endif

	


//	return y1*1000;

}





//---------------------------
// process_bmp_data3 
// new method used in v110
//---------------------------
void
process_bmp_data3(void){

	dword x1, y1, x2, y2;
	byte b1;

	x1 = (max_bmp_x - min_bmp_x)/2+min_bmp_x;
	y1 = FIRST_PT_Y;

	long temp_x, temp_y;

	temp_x = convert_2_cur_x(x1);
	temp_y = convert_2_cur_y(y1);

	cur_z = 1;
		
	if (get_xy(pt_array1, x1, y1))
	{
		goto_abs_axis_um(temp_x, temp_y, z_depth);
		set_xy(pt_array1, x1, y1, 0);
	}
	else
	{
		goto_abs_axis_um(temp_x, temp_y, z_travel_height);
	}



	//=========== main loop =======================
	b1 = 1;

	while( b1!= 0 ){

	b1 = find_next_nearest_pt(x1, y1, &x2, &y2);

		if (b1 == 0)
		{
			goto_abs_axis_um(cur_x, cur_y, z_travel_height);
			return;
		}

		else
		if (b1 == 1)
		{

#if DEBUGSTR
//			printf("(b1=1,  temp_x = %d, temp_y = %d)\n", temp_x, temp_y);
#endif

			temp_x = convert_2_cur_x(x2);
			temp_y = convert_2_cur_y(y2);

			if (cur_z > 0)
			  goto_abs_axis_um(temp_x, temp_y, z_travel_height);
			goto_abs_axis_um(temp_x, temp_y, z_depth);
			
		}
		else
		if (b1 == 2)
		{


			if (cur_z < 0)
				goto_abs_axis_um(cur_x, cur_y, z_travel_height);

			temp_x = convert_2_cur_x(x2);
			temp_y = convert_2_cur_y(y2);

#if DEBUGSTR
//			printf("(b2=1,  temp_x = %d, temp_y = %d)\n", temp_x, temp_y);
#endif
			goto_abs_axis_um(temp_x, temp_y, z_travel_height);
			goto_abs_axis_um(temp_x, temp_y, z_depth);

		}



	x1 = x2;
	y1 = y2;
	
	
	}


};
			

//---------------------------
// process_bmp_data4 
// 
//---------------------------
void
process_bmp_data4(void){

	dword x1, y1, x2, y2;
	byte b1;
	long i, j;
	long temp_x, temp_y;


	long x_adder, y_adder;
	x_adder = cnc_param1;
	y_adder = cnc_param2;

	// temp_x = convert_2_cur_x(x1);
	// temp_y = convert_2_cur_y(y1);
	// max_bmp_x-min_bmp_x


#if 1
	for(i=min_bmp_y; i<max_bmp_y; i += y_adder)


		if (i % 2)
		{
			for(j = min_bmp_x; j<= max_bmp_x; j += x_adder)
			{
				// if (j,i) == 1,  caculate (j,i) <---> (last_x1, last_y1)
				if (get_xy(pt_array1, j,i))
				{
				//	set_xy(pt_array1, j, i, 0);

					temp_x = convert_2_cur_x(j);
					temp_y = convert_2_cur_y(i);

					if (cur_z > 0)
						goto_abs_axis_um(temp_x, temp_y, z_travel_height);
					goto_abs_axis_um(temp_x, temp_y, z_depth);
						goto_abs_axis_um(temp_x, temp_y, z_travel_height);
				}
				else
				{
					//temp_x = convert_2_cur_x(j);
					//temp_y = convert_2_cur_y(i);
					temp_x = j;
					temp_y = i;

					if (cur_z < 0)
						goto_abs_axis_um(cur_x, cur_y, z_travel_height);
					goto_abs_axis_um(temp_x, temp_y, z_travel_height);

				}


			}
		}
		else
		{
			for(j = max_bmp_x; j<= min_bmp_x; j -= x_adder)
			{
				// if (j,i) == 1,  caculate (j,i) <---> (last_x1, last_y1)
				if (get_xy(pt_array1, j,i))
				{
				//	set_xy(pt_array1, j, i, 0);

					temp_x = convert_2_cur_x(j);
					temp_y = convert_2_cur_y(i);


					if (cur_z > 0)
						goto_abs_axis_um(temp_x, temp_y, z_travel_height);
					goto_abs_axis_um(temp_x, temp_y, z_depth);
						goto_abs_axis_um(temp_x, temp_y, z_travel_height);
				}
				else
				{
					temp_x = convert_2_cur_x(j);
					temp_y = convert_2_cur_y(i);

					if (cur_z < 0)
						goto_abs_axis_um(cur_x, cur_y, z_travel_height);
					goto_abs_axis_um(temp_x, temp_y, z_travel_height);

				}


			}

		}
#else


		for(i=0; i<PT_Y_LENGTH; i++)
			for(j = 0; j< PT_X_LENGTH; j++)
		//	{
				//printf("(i,j = %d, %d)\n", i, j);
				// if (j,i) == 1,  caculate (j,i) <---> (last_x1, last_y1)
			//				for(j = min_bmp_x; j<= max_bmp_x; j += x_adder)
			{
				// if (j,i) == 1,  caculate (j,i) <---> (last_x1, last_y1)
#if 1
				if (get_xy(pt_array1, j,i))
				{
				//	set_xy(pt_array1, j, i, 0);
#else
				if (isBlackPoint(j,i))
				{
#endif

					//set_xy(pt_array1, j, i, 0);

			//	temp_x =  (j*x_phisical_length/(PT_X_LENGTH));
			//		temp_y =  (i*x_phisical_length/(PT_X_LENGTH));

					temp_x = j;
					temp_y = i;

					//if (cur_z > 0)
					 	goto_abs_axis_um(temp_x, temp_y, z_travel_height);
						goto_abs_axis_um(temp_x, temp_y, z_depth);
						goto_abs_axis_um(temp_x, temp_y, z_travel_height);


				}
				else
				{

					//temp_x = convert_2_cur_x(j);
					//temp_y = convert_2_cur_y(i);

				//	temp_x =  (j*x_phisical_length/(PT_X_LENGTH));
				//	temp_y =  (i*x_phisical_length/(PT_X_LENGTH));
					temp_x = j;
					temp_y = i;

					//if (cur_z < 0)
					//	goto_abs_axis_um(cur_x, cur_y, z_travel_height);
					goto_abs_axis_um(temp_x, temp_y, z_travel_height);

				}

			}

#endif

};


	//X%d.%d Y%d.%d Z-%d.%d\n", cnc_steps++, abs_x/1000, abs_x%1000,abs_y/1000, abs_y%1000, abs_z/1000, abs(abs_z%1000)); 
void 
printf_xyz_value(long abs_x, long abs_y, long abs_z){

	float f1;
	f1 = abs_x;
	f1 = f1/1000;
	printf("X%.3f ", f1);

	f1 = abs_y;
	f1 = f1/1000;
	printf("Y%.3f ", f1);

	f1 = abs_z;
	f1 = f1/1000;
	printf("Z%.3f ", f1);


};


//---------------------
// goto_abs_axis_um
//---------------------
void 
goto_abs_axis_um(long abs_x, long abs_y, long abs_z){




	if ((abs_z < 0) && (abs(abs_z) < 1000))

	{	
		if (abs_z > 0)
		{
			printf("N%d G0 ", cnc_steps);
			//X%d.%d Y%d.%d Z-%d.%d\n", cnc_steps++, abs_x/1000, abs_x%1000,abs_y/1000, abs_y%1000, abs_z/1000, abs(abs_z%1000)); 
			printf_xyz_value(abs_x, abs_y, abs_z);
			printf("\n");

		}		
		else
		{
			printf("N%d G1 ", cnc_steps);
			printf_xyz_value(abs_x, abs_y, abs_z);
			printf("\n");

			//printf("N%d G1 X%d.%d Y%d.%d Z-%d.%d\n", cnc_steps++, abs_x/1000, abs_x%1000,abs_y/1000, abs_y%1000, abs_z/1000, abs(abs_z%1000)); 
		}
		cur_x = abs_x;
		cur_y = abs_y;
		cur_z = abs_z;

	}
	else
	{
		if (abs_z > 0)
		{	
			printf("N%d G0 ", cnc_steps);
			//X%d.%d Y%d.%d Z-%d.%d\n", cnc_steps++, abs_x/1000, abs_x%1000,abs_y/1000, abs_y%1000, abs_z/1000, abs(abs_z%1000)); 
			printf_xyz_value(abs_x, abs_y, abs_z);
			printf("\n");
		}
		else
		{
			printf("N%d G1 ", cnc_steps);
			//X%d.%d Y%d.%d Z-%d.%d\n", cnc_steps++, abs_x/1000, abs_x%1000,abs_y/1000, abs_y%1000, abs_z/1000, abs(abs_z%1000)); 
			printf_xyz_value(abs_x, abs_y, abs_z);
			printf("\n");
		}

		cur_x = abs_x;
		cur_y = abs_y;
		cur_z = abs_z;

	
	}

	cnc_steps++;

};

void
ClearBlackPoint(dword x, dword y){
byte point_map;

	if ( (x >= bmp_x_length) || (y >= bmp_y_length) )
			return;
	point_map = p_bmp_data[bmp_bytes_per_x * y + x/8];


	if ( point_map & (1 << (7- x%8)) )
	{

		return ;
	}
	else 
	{

		// remove this pt from bit map
		point_map |= (1 << (7- x%8));
		p_bmp_data[bmp_bytes_per_x * y + x/8] = point_map;


		return;
	}

}
//---------------------------
// isBlackPoint 
//---------------------------
byte
isBlackPoint(dword x, dword y){
//byte bmp_line_data[1024];
byte point_map;

	if ( (x >= bmp_x_length) || (y >= bmp_y_length) )
			return 0;
	point_map = p_bmp_data[bmp_bytes_per_x * y + x/8];

#if DEBUGSTR
	printf("offset = %d, point_map = 0x%x, x=%d, y=%d\n", bmp_bytes_per_x * y + (x)/8, point_map, x, y);
#endif

	if ( point_map & (1 << (7- x%8)) )
	{
#if DEBUGSTR
		printf("is while point\n");
#endif
		return 0;
	}
	else 
	{
#if 0
		// remove this pt from bit map
		point_map |= (1 << (7- x%8));
		p_bmp_data[bmp_bytes_per_x * y + x/8] = point_map;
#if DEBUGSTR
		printf("new byte = %x\n", p_bmp_data[bmp_bytes_per_x * y + x/8] );
#endif
#endif

		return 1;
	}

}	





