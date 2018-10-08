function [rules,vars] = MMakefile_fsolver (varargin)

    options.DoCrossBuildWin64 = false;
    options.W64CrossBuildMexLibsDir = '';
    options.Verbose = false;
    options.Debug = false;
    
    options = mmake.parse_pv_pairs (options, varargin);

    if isunix || mfemmdeps.isoctave
        ismscompiler = false;
    else
        cc = mex.getCompilerConfigurations ('C');
        if strncmpi (cc.Manufacturer, 'Microsoft', 9)
            ismscompiler = true;
        else
            ismscompiler = false;
        end
    end
    
    thisfilepath = mfemmdeps.getmfilepath (mfilename);
    
    thisfilepath = strrep (thisfilepath, '\', '/');

    vars.LDFLAGS = '${LDFLAGS} -lstdc++ -Wl,--no-undefined';

    % flags that will be passed direct to mex
    vars.MEXFLAGS = ['${MEXFLAGS} -D_GLIBCXX_USE_CXX11_ABI=1 -I"../cfemm/fsolver" -I"../cfemm/libfemm" -I"../cfemm/libfemm/liblua" '];
    
    if options.Verbose
        vars.MEXFLAGS = [vars.MEXFLAGS, ' -v'];
    end
    
    if isunix && ~mfemmdeps.isoctave ()
        if options.Debug
            vars.OPTIMFLAGS = '-OO';
            vars.MEXFLAGS = [vars.MEXFLAGS, ' CXXOPTIMFLAGS="-O0 -DDEBUG"'];
        else
            vars.OPTIMFLAGS = '-O2';
            vars.MEXFLAGS = [vars.MEXFLAGS, ' CXXOPTIMFLAGS="-O2 -DNDEBUG"'];
        end
    end
    
    if options.Verbose
        vars.MEXFLAGS = [vars.MEXFLAGS, ' -v '];
    end
    
    vars.CXXFLAGS = '${CXXFLAGS}';
    
    if mfemmdeps.isoctave
        setenv('CFLAGS','-std=c++11'); %vars.CXXFLAGS = [vars.CXXFLAGS, ' -std=c++11'];
        setenv('CXXFLAGS','-std=c++11');
        vars.MEXFLAGS = [vars.MEXFLAGS, ' ''-Wl,--no-undefined'' -lstdc++'];
    end
%     vars.CXXFLAGS = [vars.CXXFLAGS, ' -std=c++14'];
    
    if ~ismscompiler
        vars.CXXFLAGS = [vars.CXXFLAGS, ' -fpermissive'];
        vars.CFLAGS = '${CFLAGS} -fpermissive';
    end
    
    cfemmpath = fullfile (thisfilepath, '..', 'cfemm');
    fsolverpath = fullfile (cfemmpath, 'fsolver'); 
    libfemmpath = fullfile (cfemmpath, 'libfemm');
    libluacomplexpath = fullfile (libfemmpath, 'liblua');
    
    [libluacomplex_sources, libluacomplex_headers] = getlibluasources ();
    
    libfemm_sources = getlibfemmsources ();
    
    fsolver_sources = { ...
        'fsolver.cpp', ...
        'harmonic2d.cpp', ...
        'harmonicaxi.cpp', ...
        'static2d.cpp', ...
        'staticaxi.cpp', ...
    };

    [ libluacomplex_objs,  libluacomplex_rules ] = ... 
         mmake.sources2rules ( libluacomplex_sources, ...
                               'SourceDir', libluacomplexpath );
                                            
    [ libfemm_objs, libfemm_rules ] = ...
         mmake.sources2rules ( libfemm_sources, ...
                               'SourceDir', libfemmpath );
                           
    [ fsolver_objs, fsolver_rules ] = ...
         mmake.sources2rules ( fsolver_sources, ...
                               'SourceDir', fsolverpath );
          
    vars.OBJS = [ libluacomplex_objs, ...
                  libfemm_objs, ...
                  fsolver_objs, ...
                  {'mexfsolver.cpp'}, ...
                ];
            

    if options.DoCrossBuildWin64 
        
        vars = mfemmdeps.mmake_check_cross (options.W64CrossBuildMexLibsDir, vars);
        
    end

    rules = mfemmdeps.mmake_rule_1 ( 'mexfsolver', ...
                                     'DoCrossBuildWin64', options.DoCrossBuildWin64 );
    rules(1).deps = vars.OBJS;

    
    rules = [ rules, fsolver_rules, libfemm_rules, libluacomplex_rules ];

    rules(end+1).target = 'tidy';
    rules(end).commands = { 'try; delete(''../cfemm/libfemm/liblua/*.${OBJ_EXT}''); catch; end;', ...
                            'try; delete(''../cfemm/libfemm/*.${OBJ_EXT}''); catch; end;', ...
                            'try; delete(''../cfemm/fmesher/*.${OBJ_EXT}''); catch; end;', ...
                            'try; delete(''*.${OBJ_EXT}''); catch; end;' };
    tidyruleind = numel (rules);
    
    rules(end+1).target = 'clean';
    rules(end).commands = [ rules(tidyruleind).commands, ...
                            {'try; delete(''*.${MEX_EXT}''); catch; end;'} ];

end
