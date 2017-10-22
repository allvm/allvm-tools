; Test static destructor support

; TODO: Test behavior w/alley and AOT

; For now, focus on allmux

; RUN: rm -rf %t && mkdir -p %t/mux
; RUN: bc2allvm %s -o %t/test
; RUNX: alley %t/test |& FileCheck %s

; RUN: allmux %t/test -o %t/mux/test
; RUN: alley %t/mux/test |& FileCheck %s
; When program takes an argument it calls exit() instead of returning from main
; RUN: alley %t/mux/test 1 |& FileCheck %s

; Is order guaranteed?
; CHECK: ctor
; CHECK: ctor
; CHECK: main
; CHECK: dtor
; CHECK: dtor


; ModuleID = 'dtor_exit.bc'
source_filename = "dtor_exit.cpp"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-musl"

%struct.D = type { i8 }

$_ZN1DD2Ev = comdat any

@_ZL1d = internal global %struct.D zeroinitializer, align 1
@__dso_handle = external global i8
@llvm.global_ctors = appending global [2 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @_ZL9init_attrv, i8* null }, { i32, void ()*, i8* } { i32 65535, void ()* @_GLOBAL__sub_I_dtor_exit.cpp, i8* null }]
@llvm.global_dtors = appending global [1 x { i32, void ()*, i8* }] [{ i32, void ()*, i8* } { i32 65535, void ()* @_ZL9fini_attrv, i8* null }]
@str = private unnamed_addr constant [12 x i8] c"ctor D::D()\00"
@str.5 = private unnamed_addr constant [13 x i8] c"dtor D::~D()\00"
@str.6 = private unnamed_addr constant [17 x i8] c"ctor init_attr()\00"
@str.7 = private unnamed_addr constant [17 x i8] c"dtor fini_attr()\00"
@str.8 = private unnamed_addr constant [5 x i8] c"main\00"

; Function Attrs: sspstrong uwtable
define linkonce_odr void @_ZN1DD2Ev(%struct.D* %this) unnamed_addr #0 comdat align 2 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([13 x i8], [13 x i8]* @str.5, i64 0, i64 0))
  ret void
}

; Function Attrs: nounwind
declare i32 @__cxa_atexit(void (i8*)*, i8*, i8*) local_unnamed_addr #1

; Function Attrs: nounwind sspstrong uwtable
define internal void @_ZL9init_attrv() #2 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @str.6, i64 0, i64 0))
  ret void
}

; Function Attrs: nounwind sspstrong uwtable
define internal void @_ZL9fini_attrv() #2 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([17 x i8], [17 x i8]* @str.7, i64 0, i64 0))
  ret void
}

; Function Attrs: norecurse sspstrong uwtable
define i32 @main(i32 %argc, i8** nocapture readnone %argv) local_unnamed_addr #3 {
entry:
  %puts = tail call i32 @puts(i8* getelementptr inbounds ([5 x i8], [5 x i8]* @str.8, i64 0, i64 0))
  %cmp = icmp sgt i32 %argc, 1
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %entry
  tail call void @exit(i32 0) #5
  unreachable

if.end:                                           ; preds = %entry
  ret i32 0
}

; Function Attrs: noreturn
declare void @exit(i32) local_unnamed_addr #4

; Function Attrs: sspstrong uwtable
define internal void @_GLOBAL__sub_I_dtor_exit.cpp() #0 section ".text.startup" {
entry:
  %puts.i.i = tail call i32 @puts(i8* getelementptr inbounds ([12 x i8], [12 x i8]* @str, i64 0, i64 0))
  %0 = tail call i32 @__cxa_atexit(void (i8*)* bitcast (void (%struct.D*)* @_ZN1DD2Ev to void (i8*)*), i8* getelementptr inbounds (%struct.D, %struct.D* @_ZL1d, i64 0, i32 0), i8* nonnull @__dso_handle) #1
  ret void
}

; Function Attrs: nounwind
declare i32 @puts(i8* nocapture readonly) #1

attributes #0 = { sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="4" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind }
attributes #2 = { nounwind sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="4" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { norecurse sspstrong uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="4" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { noreturn "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="4" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #5 = { noreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"PIC Level", i32 2}
!1 = !{!"clang version 4.0.1 (tags/RELEASE_401/final)"}
