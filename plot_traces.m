clear
%Reram_hello = dir('*World*.csv');
Reram_file_1 = dir('*_0x0F_*.csv');
Reram_file_2 = dir('*_0x1F_*.csv');
Reram_file_3 = dir('*_0x81_*.csv');
Reram_file_4 = dir('*_0x17_*.csv');

% Array_Reram_hello = csvread(Reram_hello(1).name,1);
for index = 1:10
    Reram_1(:,index) = csvread(Reram_file_1(index).name,1);
    Reram_file_1(index).name
end

for index = 1:10
    Reram_2(:,index) = csvread(Reram_file_2(index).name,1);
    Reram_file_2(index).name
end

for index = 1:10
    Reram_3(:,index) = csvread(Reram_file_3(index).name,1);
    Reram_file_3(index).name
end
for index = 1:10
    Reram_4(:,index) = csvread(Reram_file_4(index).name,1);
    Reram_file_4(index).name
end

figure;
strips(Reram_1)
figure;
strips(Reram_2)
figure;
strips(Reram_3)
figure;
strips(Reram_4)
