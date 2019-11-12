clear
import mlreportgen.dom.*;
%%

array_database_peaks = zeros(16,2560);
% array_database_peaks_VnA = strings(1,2560);

%% create data base patterns peaks file

%loop 256
col=0;
for index = 72:79
    
   %convert index to data value (HEX) & get data value 10 traces  
   index_hex = dec2hex(index,2);
   tmp_value = ["*_0x",index_hex,"_*.csv"];
   value = join(tmp_value,"");
   array_10_trace = dir(char(value));
   
   
   for index_10 = 1:10
       
      %open 10 csv files
      col = (index*10)+index_10;
      trace = csvread(array_10_trace(index_10).name,1);
      snapshot = trace(1:90000);
      
      %fill each column with 16 paeks from data trace  
      array_database_peaks(:,col) = split_patterns_peaks_reram(snapshot);
      
      %array_database_peaks_VnA(col) = array_10_trace(index_10).name;
   end
end


%% save array datapeaks as csv

% csvwrite('array_database_peaks_REram_1.csv',array_database_peaks);
% %%
% file = fopen('array_database_peaks_sram_VnA.txt','w');
% fprintf(file,'%s\n',array_database_peaks_VnA(:));
% fclose(file);