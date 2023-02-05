## bin2c.pl / (c) Sw1ft
# A simple Perl script that converts a given file to a sequence of bytes of C/C++ source code
# Usage: perl bin2c.pl <target filename> <output filename w/o extension> <data variable name>
#

# Arguments
$input = $ARGV[0]; # input filename
$output = $ARGV[1]; # output filename
$var_name  = $ARGV[2]; # name of variable

if ( open(hInput, "<" . $input) )
{
	binmode( hInput );

	if ( open(hOutput, ">" . $output . ".c") )
	{
		if ( open(hOutputHeader, ">" . $output . ".h") )
		{
			printf( hOutputHeader "#ifndef __%s__H\n#define __%s__H\n\n#ifdef _WIN32\n#pragma once\n#endif\n\n", $var_name, $var_name );
			printf( hOutputHeader "#ifdef __cplusplus\n\nextern \"C\" char %s_bytes[];\nextern \"C\" unsigned int %s_size;\n\n", $var_name, $var_name );
			printf( hOutputHeader "#else\n\nextern char %s_bytes[];\nextern unsigned int %s_size;\n\n#endif\n\n#endif", $var_name, $var_name );
			
			close( hOutputHeader );
		}
	
		print( "Converting file \"" . $input . "\" to \"" . $output . "\" as C/C++ source code...\n" );
	
		$counter = 0;
		$size = 0;
		$separate_bytes = 0;
		$break_line_bytes = 32;
		
		printf( hOutput "char %s_bytes[] =\n{\n\t", $var_name );
		
		while ( !eof( hInput ) )
		{
			if ( $counter >= $break_line_bytes )
			{
				print( hOutput ", \n\t" );
			
				$counter = 0;
			}
			elsif ( $separate_bytes )
			{
				print( hOutput ", " );
			}
			
			printf( hOutput "0x%02X", ord( getc(hInput) ) );
			
			$separate_bytes = 1;
			$size++;
			$counter++;
		}
		
		printf( hOutput "\n};\n\nunsigned int %s_size = %d;", $var_name, $size );
		
		printf( "Done. (bytes: %d)\n", $size );
		
		close( hOutput );
	}

	close( hInput );
}