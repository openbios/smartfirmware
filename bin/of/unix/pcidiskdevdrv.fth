\ Driver for the fake disk device 

fcode-version2

\ set standard properties
" pcidiskdev" device-name
" block" device-type
" CODEGEN,pcidiskdev" model

headerless
instance variable g_reg 0 g_reg l!
instance variable g_regsize
instance variable g_disk_blocks 0 g_disk_blocks l!
instance variable g_deblocker_pkg 0 g_deblocker_pkg l!
instance variable g_disklabel_pkg 0 g_disklabel_pkg l!

\ @0		command/status reg
\			commands
\			0000.xxxx		no op
\			0001.xxxx		read id info
\			0011.xxxx		read block
\			0012.xxxx		write block
\			0021.xxxx		read blocks
\			0022.xxxx		write blocks
\			status
\			0000.0001		operation complete
\			0000.0002		error durring op
\			0000.0004		interrupt pending
\ @4		sector register
\ @8		DMA address register
\ @C		block count register
\ @10		error code
\ @1C		Emulator Rev
\ @20		interrupt status
\ @24		interrupt mask
\ @30		drive select

: reg-read		( off -- data )
	g_reg l@ + rl@
;

: reg-write		( data off -- )
	g_reg l@ + rl!
;

: write-command		( cmd -- )
	." PCIDISKDEV: write-command " dup . cr
	0 reg-write
;

: read-status		( -- status )
	0 reg-read
;

: wait-complete		( -- status )
	1000 0 do
		read-status		( status )
		dup			( status status )
		3 and			( status lowbits )
		0 <>			( status done? )
		if true else drop false then
		?leave
	loop
;

: cmd-read-id		( -- status )
	0001.0000 write-command wait-complete
;

: cmd-read-block	( -- status )
	0011.0000 write-command wait-complete
;

: cmd-write-block	( -- status )
	0012.0000 write-command wait-complete
;

: cmd-read-blocks	( -- status )
	0021.0000 write-command wait-complete
;

: cmd-write-blocks	( -- status )
	0022.0000 write-command wait-complete
;

: set-block-number	( block-numer -- )
	4 reg-write
;

: set-dma-address	( dma-address -- )
	8 reg-write
;

: set-block-count	( #blocks -- )
	C reg-write
;

: set-drive-sel		( drive -- )
	1 swap lshift
	30 reg-write
;

\ device node convience functions
: set-address 		( address -- )
	dup if
		dup encode-int " address" property
	else
		" address" delete-property
	then
	g_reg l!
;

: map-decode-reg	( prop len -- prop len sizelo sizehi physlo physmid physhi )
	decode-int >r
	decode-int >r
	decode-int >r
	decode-int >r
	decode-int >r
	r> r> r> r> r>
;

: map-find-bar0		( prop len -- false | plo pmid phi size true )
	false -rot		( flase prop len )
	begin dup		( len != 0 )
	while
		map-decode-reg 	( prop len slo shi plo pmid phi )
		dup ff and 10 = if
			>r >r >r drop >r 3drop 		( )
			r> r> swap r> swap r>		( plo pmid size phi )
			dup -rot			( plo pmid phi size phi )
			\ check space bits
			0f00.0000 and 0200.0000 = 	( plo pmid phi size mem? )
			0 0
		else
			3drop 2drop 
		then
	repeat
	2drop
;

: map-in-device		( -- )
	" reg" get-my-property			( prop len err )
	if 0 set-address ." no prop" exit then		( prop len )
	map-find-bar0				( plo pmid phi sz mem? )
	not if 0 set-address exit then		( plo pmid phi sz )
	dup g_regsize l!
	" map-in" $call-parent			( address )
	set-address
;

: map-out-device	( -- )
	g_reg l@ ?dup if g_regsize l@ " map-out" $call-parent then
	0 set-address
;

: config-l@		( physhi -- value )
	" config-l@" $call-parent
;

: config-l!		( value physhi -- )
	" config-l!" $call-parent
;

: enable-address-space	( -- )
	my-space FF not and 4 or	( physhi )
	dup config-l@			( physhi command )
	FFFF and 7 or 			( physhi command )
	swap config-l!			( )
;

: disable-address-space	( -- )
	my-space FF not and 4 or	( physhi )
	dup config-l@ 			( physhi command )
	FFF8 and 			( physhi command )
	swap config-l!			( )
;

: print-rev			( rev -- )
	dup 1.0000 / dup <# u# dup if u# then u#> type ." ." 
	1.0000 * - 100 / <# u# u# u#> type cr
;

: show-stack			( ... - ... )
	." PCIDISKDEV: Current Stack" cr
	" .s" eval
;

headers
: open
	\ need to map in registers
	." PCIDISKDEV: open" cr
	map-in-device
	." PCIDISKDEV: map-in called" cr

	g_reg l@ 0 = if
		false exit
	then

	enable-address-space
	." PCIDISKDEV: enable-space called" cr
	." PCI Disk Device Emulation Rev " 
	1C reg-read print-rev cr
	1 set-drive-sel	

	\ install deblocker package and disklabel package
	" " " deblocker" $open-package g_deblocker_pkg l!
	." PCIDISKDEV: deblocker opened" cr
	" " " disk-label" $open-package g_disklabel_pkg l!
	." PCIDISKDEV: disklabel opened" cr
	true
;

: close
	." PCIDISKDEV: close" cr
	\ close deblocker and disklabel packages
	g_disklabel_pkg l@ close-package 0 g_disklabel_pkg l!
	g_deblocker_pkg l@ close-package 0 g_deblocker_pkg l!

	\ unmap registers
	disable-address-space
	map-out-device
;

: check-status		( #blocks status -- #blocks )
	2 and 0 <> if drop 0 then
;

: read-blocks		( addr block# #blocks -- #read )
	dup set-block-count		( addr block# #blocks )
	swap set-block-number		( addr #blocks )
	dup -rot d# 512 * true		( #blocks addr numbytes true )
	" dma-map-in" $call-parent	( #blocks pciaddr )
	set-dma-address			( #blocks )
	cmd-read-blocks			( #blocks status )
	check-status
;

: write-blocks
	dup set-block-count		( addr block# #blocks )
	swap set-block-number		( addr #blocks )
	swap set-dma-address		( #blocks )
	cmd-write-blocks		( #blocks status )
	check-status
;

: block-size				( -- block-len )
	d# 512
;

: max-transfer				( -- max-transfer-len )
	block-size d# 8 * 		( 8 blocks max )
;

: #blocks				( -- disk-size-in-blocks )
	g_disk_blocks l@
;

: size					( -- disk-size-in-bytes )
	block-size #blocks *
;

: read
	" read" g_deblocker_pkg l@ $call-method
;

: write
	" write" g_deblocker_pkg l@ $call-method
;

: seek
	" seek" g_deblocker_pkg l@ $call-method
;

: load
	." PCIDISKDEV: load method  " g_disklabel_pkg l@ . cr
	
	" load" g_disklabel_pkg l@ $call-method
	." PCIDISKDEV: load method done" cr
;

: selftest
	0
;

: encode-csr		( -- prop len )
	my-address
	my-space
	encode-phys
	0 encode-int encode+
	0 encode-int encode+
;

: bar-size 		( bar-offset -- size )
	dup my-space + swap
	30 = if 400 else
	    dup config-l@ 1 and 0= if
		10
	    else
		4
	    then
	then
	swap			( gran physhi )
	dup config-l@ swap	( gran old-bar physhi )
	-2 over config-l!
	dup config-l@ 		( gran old-bar physhi new-bar )
	-rot			( gran new-bar old-bar physhi )
	config-l!		( gran new-bar )
	not swap 1 - or 1 + 	( size )
;

: encode-bar		( prop len bar -- prop len )
	dup bar-size >r		( prop len bar )
	my-space +
	200.0000 +
	my-address rot
	encode-phys encode+
	0 encode-int encode+
	r> encode-int encode+
;

: make-reg-prop			( -- )
	encode-csr
	10 encode-bar
	30 encode-bar
	" reg" property
;

: probe				( -- found )
	\ read the device ID and print out the information
	cr
	." PCI Disk Device -- probing " 

	make-reg-prop
	" assign-addresses" eval

	map-in-device

	g_reg l@ 0 = if
		." -- device not found" cr
		false exit
	then

	enable-address-space

	cr 
	." PCI Disk Device Emulation Rev " 
	1C reg-read print-rev
	." PCI Disk Device FCode Driver Rev 1.04" cr

	disable-address-space
	map-out-device
	cr
	true
;

probe drop
fcode-end
