\ Driver for the fake ethernet device 
\ Copyright 2003 (C) CodeGen, Inc.
\ All rights reserved.

fcode-version2

\ set standard properties
" pcietherdev" device-name
" block" device-type
" CODEGEN,pcietherdev" model

headerless
400 constant link-stat
404 constant link-ctl

410 constant mac-low
414 constant mac-high

800 constant rx-stat
804 constant rx-ctl
808 constant rx-base
80C constant rx-size
810 constant rx-head
814 constant rx-tail

C00 constant tx-stat
C04 constant tx-ctl
C08 constant tx-base
C0C constant tx-size
C10 constant tx-head
C14 constant tx-tail

variable g_reg 0 g_reg !
variable g_regsize
instance variable g_obptftp_pkg 0 g_obptftp_pkg !

variable g_rxdesc 0 g_rxdesc !
variable g_rxdescbuf 0 g_rxdescbuf !
variable g_numrxdesc
variable g_rxbuf 0 g_rxbuf !
variable g_rxbufsize

variable g_txdesc 0 g_txdesc !
variable g_txdescbuf 0 g_txdescbuf !
variable g_numtxdesc
variable g_txbuf 0 g_txbuf !
variable g_txbufsize

variable g_txtail 0 g_txtail !
variable g_rxhead 0 g_rxhead !

6 buffer: g_macaddr

: dma-alloc ( size -- addr )
	" dma-alloc" $call-parent
;

: dma-free ( addr size -- )
	" dma-free" $call-parent
;

: rx-alloc ( bufsize bufcount -- )
\	." Start rx-alloc: "
\	2dup swap ." bufsize " u. ." bufcount " u. cr

	\ Allocate DMA buffers
	2dup g_numrxdesc ! g_rxbufsize !		( bufsize bufcount )
	* dma-alloc g_rxbuf !
	g_numrxdesc @ 8 * 7 + dma-alloc g_rxdescbuf !
	g_rxdescbuf @ 7 + 7 not and g_rxdesc !		\ Align address

	\ Initialize the Rx descriptors
	g_numrxdesc @ 0 do 
		g_rxdesc @ i 8 * +
\		rxdesc[i].len = g_rxbufsize;
		dup g_rxbufsize @ swap 2 + w!
\		rxdesc[i].flags = 0;
		dup 0 swap w!
\		rxdesc[i].addr = rxbuf[i * g_rxbufsize];
		4 + g_rxbuf @ i g_rxbufsize @ * + swap l!

	loop
;

: rx-free
	g_rxbuf @ g_numrxdesc @ g_rxbufsize @ * dma-free 0 g_rxbuf !
	g_rxdescbuf @ g_numrxdesc @ 8 * 7 + dma-free 0 g_rxdescbuf !
	0 g_rxdesc !
	0 g_numrxdesc !
	0 g_rxbufsize !
;

: rx-dump
	." RX DESCRIPTORS " g_rxdesc @ u. cr
	g_numrxdesc @ 0 do
		g_rxdesc @ i 8 * +
		." #" i u.
		."  : flags " dup w@ u. 
		."  : len " dup 2 + w@ u.
		."  : ptr " 4 + l@ u. cr
	loop
;

: tx-alloc ( bufsize bufcount -- )
\	." Start tx-alloc: "
\	2dup swap ." bufsize " u. ." bufcount " u. cr

	\ Allocate DMA buffers
	2dup g_numtxdesc ! g_txbufsize !		( bufsize bufcount )
	* dma-alloc g_txbuf !
	g_numtxdesc @ 8 * 7 + dma-alloc g_txdescbuf !
	g_txdescbuf @ 7 + 7 not and g_txdesc !		\ Align address

	\ Initialize the Rx descriptors
	g_numtxdesc @ 0 do 
		g_txdesc @ i 8 * +
\		txdesc[i].len = g_txbufsize;
		dup g_txbufsize @ swap 2 + w!
\		txdesc[i].flags = 1;
		dup 1 swap w!
\		txdesc[i].addr = txbuf[i * g_txbufsize];
		4 + g_txbuf @ i g_txbufsize @ * + swap l!

	loop
;

: tx-free
	g_txbuf @ g_numtxdesc @ g_txbufsize @ * dma-free 0 g_txbuf !
	g_txdescbuf @ g_numtxdesc @ 8 * 7 + dma-free 0 g_txdescbuf !
	0 g_txdesc !
	0 g_numtxdesc !
	0 g_txbufsize !
;

: tx-dump
	." TX DESCRIPTORS " g_txdesc @ u. cr
	g_numtxdesc @ 0 do
		g_txdesc @ i 8 * +
		." #" i u.
		."  : flags " dup w@ u. 
		."  : len " dup 2 + w@ u.
		."  : ptr " 4 + l@ u. cr
	loop
;

: reg-read		( off -- data )
	g_reg @ + rl@
;

: reg-write		( data off -- )
	g_reg @ + rl!
;

: link-stat@		( -- link-stat )
    link-stat reg-read
;

: link-ctl@		( -- link-ctl )
    link-ctl reg-read
;

: link-ctl!		( link-ctl -- )
    link-ctl reg-write
;

\ enable the link
: link-enable		( -- )
    \ check if link enabled
    link-ctl@ 1 and 0= if 
	\ enable the link
	link-ctl@ 1 or link-ctl!
    then

    100 0 do
	link-stat@ drop
    loop
;

: rx-setup
	d# 2048 d# 4 rx-alloc
	rx-dump

\	." In rx-setup" cr
\	." g_reg " g_reg @ u. cr
	0 rx-ctl reg-write
	g_rxdesc @ rx-base reg-write
	g_numrxdesc @ 8 * rx-size reg-write
	g_rxdesc @ rx-head reg-write
	g_rxdesc @ rx-tail reg-write
	1 rx-ctl reg-write
	g_rxdesc @ g_rxhead !
;

: tx-setup
	d# 2048 d# 4 tx-alloc
	tx-dump

	0 tx-ctl reg-write
	g_txdesc @ tx-base reg-write
	g_numtxdesc @ 8 * tx-size reg-write
	g_txdesc @ tx-head reg-write
	g_txdesc @ tx-tail reg-write
	1 tx-ctl reg-write
	g_txdesc @ g_txtail !
;

\ device node convience functions
: set-address 		( address -- )
	dup if
		dup encode-int " address" property
	else
		" address" delete-property
	then
	g_reg !
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
	dup g_regsize !
	" map-in" $call-parent			( address )
	set-address
;

: map-out-device	( -- )
	g_reg @ ?dup if g_regsize @ " map-out" $call-parent then
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
	." PCIETHERDEV: Current Stack" cr
	" .s .rs" eval
;

: read-mac-addr
    mac-low reg-read
    dup g_macaddr 5 + c!
    dup 8 rshift g_macaddr 4 + c!
    dup 16 rshift g_macaddr 3 + c!
    24 rshift g_macaddr 2 + c!
    mac-high reg-read
    dup g_macaddr 1 + c!
    8 rshift g_macaddr 0 + c!
    g_macaddr 6
;

headers
: open
	\ need to map in registers
\	." PCIETHERDEV: open" cr
	map-in-device
\	." PCIETHERDEV: map-in called" cr

	g_reg @ 0 = if
		false exit
	then

	enable-address-space
\	." PCIETHERDEV: enable-space called" cr
	." PCI Ether Device Emulation Rev " 
	1C reg-read print-rev 

	\ check for link up
	link-enable

	\ init receiver and transmitter
	rx-setup
	tx-setup

	read-mac-addr encode-bytes " local-mac-address" property

	\ install obptftp package 
	my-args " obp-tftp" $open-package g_obptftp_pkg !
\	." PCIETHERDEV: obptftp opened" cr
	true
;

: close
\	." PCIETHERDEV: close" cr
	g_reg @ 0 = if
		exit
	then

\	." PCIETHERDEV: closing obptftp" cr
	\ close obptftp package
	g_obptftp_pkg @ ?dup if 
		close-package 0 g_obptftp_pkg !
	then

\	." PCIETHERDEV: disabling receiver and transmitter" cr
	\ disable receiver and transmitter
	0 rx-ctl reg-write
	0 tx-ctl reg-write

\	." PCIETHERDEV: freeing descriptors" cr
	\ free buffers and descriptors
	rx-free
	tx-free

	\ unmap registers
\	." PCIETHERDEV: disabling address space" cr
	disable-address-space
\	." PCIETHERDEV: mapping out address space" cr
	map-out-device
;

: != 
    <>
;

headerless
: rx-next ( addr -- next-addr )
    8 + dup
    g_rxdesc @ g_numrxdesc @ 8 * + = if
	drop g_rxdesc @
    then
;

: tx-next ( addr -- next-addr )
    8 + dup
    g_txdesc @ g_numtxdesc @ 8 * + = if
	drop g_txdesc @
    then
;

: buf-adjust	( addr len desc buf buflen - naddr nlen desc addr buf copylen )
	3 pick min		( addr len desc buf copylen )
	-rot >r >r dup >r	( addr len copylen ) ( R: buf desc copylen )
	2dup -			( addr len copylen nlen )
	-rot drop		( addr nlen copylen )
	rot dup >r 		( nlen copylen addr ) ( R: buf desc copylen addr )
	+ swap			( naddr nlen )
	r> r>	r> -rot		( naddr nlen desc addr copylen ) ( R: buf )
	r> swap 		( naddr nlen desc addr buf copylen )
;

headers
: read		( addr len -- len )
\	." PCIETHERDEV: read method  " 2dup swap 
\	." addr " u. ."  len " u. cr
\	show-stack

	\ return -2 if no packets
	rx-tail reg-read
\	." rx-tail " dup u. cr
	g_rxhead @ 
\	." g_rxhead " dup u. cr
	= 
\	." rx-tail = g_rxhead " dup u. cr
	if 
		2drop -2 exit
	then

\	." PCIETHERDEV: read method: we have packets" cr
	cr ." P"

	\ check the descriptor to make sure that it has a packet
	g_rxhead @ w@		( addr len flags )
\	." flags " dup u. cr

	dup 1 and 0= if
	    \ Host bit is not set
	    ." PCIETHERDEV: read method: not yet transfered to host" cr
	    drop 2drop -1 exit
	then

	2 and 0= if
	    \ SOP bit is not set
	    ." PCIETHERDEV: read method: SOP not set" cr
	    drop 2drop -1 exit
	then

	\ save away initial length
	0 
	g_rxhead @		( addr len alen desc )

\	." PCIETHERDEV: read method: first desc " dup u. cr
\	show-stack

	begin 
		\ check if usable
		dup rx-tail reg-read !=

	        dup if ." n" then

		\ check if host owned
		over w@ 1 and 0<> 
	        dup if ." h" then
		and

		\ check if len is non-zero
		3 pick 0<> 
	        dup if ." l" then
		and

	    while
\		." PCIETHERDEV: read method: copy desc " cr
\		show-stack

		swap >r		( addr len desc R: alen )
		\ copy in the packet
		dup 4 + l@	( addr len desc buf )
		over 2 + w@ 3FFF and ( addr len desc buf buflen )
		dup r> + >r	\ increment alen
		buf-adjust	( addr len desc dest buf copylen )
		-rot swap rot	( addr len desc buf dest copylen )
		move		( addr len desc )
		r> swap		( addr len alen desc )

\		." PCIETHERDEV: read method: copy done " cr
\		show-stack

		2 pick 0= if 
		    \ were out of buffer space make sure this is the
		    \ end of packet
		    ." b"

		    \ get the flags
		    dup w@		( addr len alen desc flags )
		    4 and 0= if
			\ EOP isn't set
			." PCIETHERDEV: read method: missing EOP " cr
			swap drop 0 swap
		    then
		else
		    \ get the flags
		    ." m"
		    dup w@		( addr len alen desc flags )
		    4 and 0<> if
			\ EOP is set clear len
			." e"
			rot drop 0 -rot
		    then
		then
		
		\ reset length in descriptor
		g_rxbufsize @ over 2 + w!	
		\ clear the flags
		0 over w!

		\ adjust the descriptor pointer
		rx-next
	repeat

\	." PCIETHERDEV: read method: next desc " dup u. cr
\	show-stack

	\ save away alen and the new head
	swap >r >r

	0<> if
		." PCIETHERDEV: read method: incomplete packet" cr
		\ we ran out of descriptors before the end of the packet 
		r> r> 		( addr desc len )
		2drop drop 0 exit
	then

	drop r> r> swap		( len newdesc )

\	." PCIETHERDEV: read method: updating rx-head" cr
\	show-stack

	\ update the head
	dup rx-head reg-write	( len newdesc )
	g_rxhead !		( len )

\	." PCIETHERDEV: read method: done" cr
\	show-stack
;

\	Tx head points to the next entry to be written with a packet.
\	Tx tail points to the next packet to be sent or being sent.

: write		( addr len -- len )
\	." PCIETHERDEV: write method  " 2dup swap 
\	." addr " u. ."  len " u. cr
\	." PCIETHERDEV: write start" cr
\	show-stack

	\ make sure the packet is at least the minimum size
	dup d# 60 < if 
	    ." PCIETHERDEV: write method: too small" cr
	    drop 0 exit
	then

\	." PCIETHERDEV: write method: len good" cr 

	\ save away length
	dup >r
	g_txtail @		( addr len desc )

\	." PCIETHERDEV: write method: first desc " dup u. cr
\	show-stack

	begin 
		\ check if usable
		dup tx-next tx-head reg-read !=

		\ check if host owned
		over w@ 1 and 0<> and

		\ check if len is non-zero
		2 pick 0<> and

	    while
\		." PCIETHERDEV: write method: copy desc " cr
\		show-stack

		\ copy in the packet
		dup 4 + l@	( addr len desc buf )
		g_txbufsize @	( addr len desc buf buflen )
		buf-adjust	( addr len desc src buf copylen )
		dup 4 pick 2 + w!	\ put length in descriptor
		move		( addr len desc )

		\ clear the flags
		0 over w!

		over 0= if
			\ set EOP
			4 over w!
		then

		\ adjust the descriptor
		tx-next
	repeat

\	." PCIETHERDEV: write method: next desc " dup u. cr
\	show-stack

	\ save away new tail
	>r

	0<> if
		." PCIETHERDEV: write method: no room" cr
		\ packet doesn't fit, error out
		r> r> 		( addr desc len )
		2drop drop 0 exit
	then

	drop r> r> swap		( len newdesc )

\	." PCIETHERDEV: write method: were ok?" cr
\	show-stack

	\ set up for the begining of the packet
	g_txtail @ dup w@	( len newdesc desc flags )
	2 or			( len newdesc desc flags )
	swap w!			( len newdesc )

\	." PCIETHERDEV: write method: updating tx-tail" cr

	\ update the tail
	dup tx-tail reg-write	( len newdesc )
	g_txtail @ swap		( len desc newdesc )
	g_txtail !		( len desc )

\	." PCIETHERDEV: write method: starting transmitter" cr
	\ start the transmitter
	5 tx-ctl reg-write

\	." PCIETHERDEV: write method: waiting for packet to send" cr
	\ wait the the transmitter to finish
	1000 0 do 
	    tx-stat reg-read 4 and 0= if
		w@ 0c0 and 0<> if drop -1 then
		leave 
	    then
	loop

\	." PCIETHERDEV: write method: done" cr
	tx-stat reg-read 4 and 0<>  if
	    2drop -1 
	then

\	." PCIETHERDEV: write end" cr
\	show-stack
;

: load
\	." PCIETHERDEV: load method  " g_obptftp_pkg @ . cr
	
	" load" g_obptftp_pkg @ $call-method
\	." PCIETHERDEV: load method done" cr
;

: selftest
	0
;

headerless
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
	." PCI Ether Device -- probing " 

	make-reg-prop
	" assign-addresses" eval

	map-in-device

	g_reg @ 0 = if
		." -- device not found" cr
		false exit
	then

	enable-address-space

	cr 
	." PCI Ether Device Emulation Rev " 
	1C reg-read print-rev
	." PCI Ether Device FCode Driver Rev 1.07" cr

	disable-address-space
	map-out-device
	cr
	true
;

probe drop
fcode-end
