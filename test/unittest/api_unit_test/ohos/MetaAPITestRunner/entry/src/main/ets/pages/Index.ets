@Entry
@Component
struct Index {
  @State message: string = "MetaAPITestRunner"
  @State durationVal: number = 1500;
  @State animationDelays: number[] = [0, this.durationVal/3, this.durationVal/3 * 2];

  // Function to generate a random hex color
  private generateRandomColor(): string {
    const randomColor = Math.floor(Math.random() * 16777215).toString(16);
    return `#${randomColor.padStart(6, '0')}`;
  }

  build() {
    Column() {
      Text(this.message)
        .fontSize(30)
        .fontWeight(FontWeight.Bold)
        .fontColor(Color.White)
        .margin({top:40})
      Row() {
        ForEach(this.animationDelays, (animationDelay: number) => {
          Row() {
            Circle()
              .width(20)
              .height(20)
              .fillOpacity(1)
              .fill(Color.White)
              .strokeWidth(1)
              .stroke(Color.White)
              .align(Alignment.Center)
              .transition(TransitionEffect.OPACITY.animation({
                duration: this.durationVal,
                curve: Curve.Smooth,
                delay: animationDelay,
                iterations: -1, // The value -1 indicates that the animation is played for an unlimited number of times.
                playMode: PlayMode.AlternateReverse,
              }).combine(TransitionEffect.scale({x:0,y:0}).animation({
                duration: this.durationVal,
                curve: Curve.Smooth,
                delay: animationDelay,
                iterations: -1, // The value -1 indicates that the animation is played for an unlimited number of times.
                playMode: PlayMode.AlternateReverse,
              })))
          }
          .width('33.33%')
          .alignItems(VerticalAlign.Center)
          .justifyContent(FlexAlign.Center)
          .margin({top:40})
        })
      }
      .width('100%')
      .alignItems(VerticalAlign.Center)
      XComponent({
        id: 'xComponentID',
        type: XComponentType.SURFACE,
        libraryname: "entry"
      })
        .width('100%')
        .height('100%')
        .margin({top:40})
        .backgroundColor(Color.Transparent)
    }
    .width('100%')
    .height('100%')
    .backgroundColor(Color.Black)
    .alignItems(HorizontalAlign.Center)
  }
}