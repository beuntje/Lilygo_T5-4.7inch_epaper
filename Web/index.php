<?php
$w    = 960;
$h    = 540;
$data = '';

if ( isset( $_GET['time'] ) ) {
	header( 'Content-Type: text/plain' );
	$midnight = mktime( 24, 0, 0, date( 'n' ), date( 'j' ), date( 'Y' ) );
	$togo     = ceil( ( $midnight - time() ) / 60 ) + 5;
	if ( $togo > ( 24 * 60 * 60 ) - 5 ) {
		$togo = 0; // voor de zekerheid toch nog eens een refresh
	}
	echo $togo;
	exit;
}

//ini_set('display_errors', 1);
//ini_set('display_startup_errors', 1);
//error_reporting(E_ALL);

$start_x = intval( $_GET['x'] );
$start_y = intval( $_GET['y'] );
$width   = intval( $_GET['w'] );
$height  = intval( $_GET['h'] );

$files = array_values( array_diff( scandir( './images' ), array( '..', '.' ) ) );
$index = round( time() / 60 / 60 / 24 ) % count( $files );

$file = $files[ $index ];

$im = crop( './images/' . $file, $w, $h );
imagefilter( $im, IMG_FILTER_GRAYSCALE );

if ( isset( $_GET['stream'] ) ) {
	header( 'Content-Type: image/png' );
	imagepng( $im );
	exit;
} else {
	header( 'Content-Type: text/plain' );
}

$prev_char  = '';
$prev_count = 1;
for ( $x = $start_x; $x < $start_x + $width; $x ++ ) {
	for ( $y = $start_y; $y < $start_y + $height; $y ++ ) {
		$rgb  = imagecolorat( $im, $x, $y );
		$red  = ( $rgb >> 16 ) & 0xFF;
		$tint = round( $red / 16 );
// $tint = round( $x/$w*16);
		$char = chr( 65 + $tint );
		if ( $prev_char === $char ) {
			$prev_count ++;
		} else {
			echo $prev_char;
			echo ( 1 === $prev_count ) ? '' : $prev_count;

			$prev_char  = $char;
			$prev_count = 1;
		}
	}
}

echo $prev_char;
echo ( 1 === $prev_count ) ? '' : $prev_count;

function crop( $file, $width, $height ) {
	$path_parts = pathinfo( $file );
	switch ( strtolower( $path_parts['extension'] ) ) {
		case 'png':
			$image = imagecreatefrompng( $file );
			break;
		default:
			$image = imagecreatefromjpeg( $file );
	}

	$w = @imagesx( $image ); //current width
	$h = @imagesy( $image ); //current height
	if ( ( ! $w ) || ( ! $h ) ) {
		$GLOBALS['errors'][] = 'Image could not be resized because it was not a valid image.';

		return false;
	}
	if ( ( $w == $width ) && ( $h == $height ) ) {
		return $image;
	} //no resizing needed

	//try max width first...
	$ratio = $width / $w;
	$new_w = $width;
	$new_h = $h * $ratio;

	//if that created an image smaller than what we wanted, try the other way
	if ( $new_h < $height ) {
		$ratio = $height / $h;
		$new_h = $height;
		$new_w = $w * $ratio;
	}

	$image2 = imagecreatetruecolor( $new_w, $new_h );
	imagecopyresampled( $image2, $image, 0, 0, 0, 0, $new_w, $new_h, $w, $h );

	//check to see if cropping needs to happen
	if ( ( $new_h != $height ) || ( $new_w != $width ) ) {
		$image3 = imagecreatetruecolor( $width, $height );
		if ( $new_h > $height ) { //crop vertically
			$extra = $new_h - $height;
			$x     = 0; //source x
			$y     = round( $extra / 2 ); //source y
			imagecopyresampled( $image3, $image2, 0, 0, $x, $y, $width, $height, $width, $height );
		} else {
			$extra = $new_w - $width;
			$x     = round( $extra / 2 ); //source x
			$y     = 0; //source y
			imagecopyresampled( $image3, $image2, 0, 0, $x, $y, $width, $height, $width, $height );
		}
		imagedestroy( $image2 );

		return $image3;
	} else {
		return $image2;
	}
}

function imagettfstroketext( &$image, $size, $angle, $x, $y, $textcolor, $strokecolor, $fontfile, $text, $px ) {
	for ( $c1 = ( $x - abs( $px ) ); $c1 <= ( $x + abs( $px ) ); $c1 ++ ) {
		for ( $c2 = ( $y - abs( $px ) ); $c2 <= ( $y + abs( $px ) ); $c2 ++ ) {
			$bg = imagettftext( $image, $size, $angle, $c1, $c2, $strokecolor, $fontfile, $text );
		}
	}

	return imagettftext( $image, $size, $angle, $x, $y, $textcolor, $fontfile, $text );
}
