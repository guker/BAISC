#!/bin/sh

link_target()		{
	remove_only=0
	if [ "$1" = "-r" ]; then
		remove_only=1
		shift
	fi
	target_name=$1
	link_name=`basename $target_name`
	link_dir=$2
	arch_fix=$3
	case "$link_name" in
		*.a)
			link_dir="${link_dir}/lib${arch_fix}"
			;;
		*)
			link_dir="${link_dir}/bin${arch_fix}"
			;;
	esac
	
	link_name="${link_dir}/$link_name"
	#unlink $link_name 2>/dev/null
	rm $link_name 2>/dev/null
	if [ $remove_only -eq 0 ] ; then
		makedir 0 $link_dir
		#ln -s $target_name $link_name
		cp $target_name $link_name
	fi
	
	if [ $remove_only -eq 1 ] ; then
		#rm $target_name
		rm $link_name
	fi
	exit $?
}

link_target_cli()		{
	remove_only=0
	if [ "$1" = "-r" ]; then
		remove_only=1
		shift
	fi
	target_name=$1
	link_name=`basename $target_name`
	link_dir=$2
	arch_fix = $3
	
	link_name="${link_dir}/$link_name"
	unlink $link_name 2>/dev/null
	if [ $remove_only -eq 0 ] ; then
		makedir 0 $link_dir
		ln -s $target_name $link_name
	fi
	
	if [ $remove_only -eq 1 ] ; then
		rm $target_name
	fi
	exit $?
}

makedir( )		 {
	file_mode=$1
	shift
	dir_name="$1"
	if [ $file_mode -eq '1' ]; then
		dir_name=`dirname $1`
	fi
	if [ ! -d "$dir_name" ]; then
		echo "Create $dir_name"
	fi
	mkdir -p $dir_name;
}

install(){
	
	name=$1
	exename=$2
	prefix=$3
	dst=""
	#echo inshow $name $exename $prefix
	if [ "$name"x = "$exename"x ];then
		dst=$prefix"/bin"
	else
		dst=$prefix"/lib"
	fi
	echo install $name ">>>>" $dst;
	if [ ! -d "$dst" ]; then
		echo "Create $dst"
		mkdir -p $dst;
	fi
	cp $name $dst;
}


uninstall(){
	
	name=$1
	exename=$2
	prefix=$3
	dst=""
	#echo inshow $name $exename $prefix
	if [ "$name"x = "$exename"x ];then
		dst=$prefix"/bin/"
	else
		dst=$prefix"/lib/"
	fi
	echo uninstall $dst$name
	rm -f $dst$name
}


action=$1
shift
case "${action}" in
	mln)
		link_target $1 $2 $3 $4
		exit $?
		;;
	mlncli)
		link_target_cli $1 $2 $3
		exit $?
		;;
	install)
		install $1 $2 $3
		;;
	uninstall)
		uninstall $1 $2 $3
		;;
	ckdir)
		fmode=0
		if [ "-f" = "$1" ]; then
			fmode=1
			shift
		fi
		for i in $@; do
			makedir $fmode $i
		done
		;;
	*)
		echo "unknown action ${action}"
esac

