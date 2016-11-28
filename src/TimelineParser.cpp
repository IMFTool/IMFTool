#include "TimelineParser.h"
#include "WidgetComposition.h"
#include "GraphicsWidgetComposition.h"
#include "GraphicsWidgetSegment.h"
#include "GraphicsWidgetSequence.h"
#include "GraphicsWidgetResources.h"

void TimelineParser::run() {

	float sequence_offset = 0;
	float segment_offset = 0;

	int parsing_ttml = 0; // time in ms
	int last_sequence = 0;

	// loop all elements currently in timeline
	for (int i = 0; i < composition->GetSegmentCount(); i++) {
		GraphicsWidgetSegment *p_segment = composition->GetSegment(i);

		if (p_segment) {
			cpl::SegmentType_SequenceListType sequence_list;
			cpl::SegmentType_SequenceListType::AnySequence &r_any_sequence(sequence_list.getAny());
			//xercesc::DOMDocument &doc = sequence_list.getDomDocument();
			for (int ii = 0; ii < p_segment->GetSequenceCount(); ii++) {
				GraphicsWidgetSequence *p_sequence = p_segment->GetSequence(ii);

				if (p_sequence && !p_sequence->IsEmpty() && p_sequence != nullptr) {

					//cpl::SequenceType_ResourceListType resource_list;
					//cpl::SequenceType_ResourceListType::ResourceSequence &resource_sequence = resource_list.getResource();
					for (int iii = 0; iii < p_sequence->GetResourceCount(); iii++) {
						AbstractGraphicsWidgetResource *p_resource = p_sequence->GetResource(iii);
						if (p_resource->type() == GraphicsWidgetVideoResourceType) {

							// VideoResource found
							GraphicsWidgetVideoResource *timelineWidget = dynamic_cast<GraphicsWidgetVideoResource*>(p_resource);

							// create new playlist element
							PlayListElement playlist_element;
							
							playlist_element.in = timelineWidget->GetFirstVisibleFrame().GetOverallFrames();
							playlist_element.out = timelineWidget->GetLastVisibleFrame().GetOverallFrames();

							if (p_resource->GetAsset()) {
								playlist_element.asset = p_resource->GetAsset();
							} // else: asset is invalid

							// add playlist element to playlist
							playlist->append(playlist_element);
						}
						else if (p_resource->type() == GraphicsWidgetTimedTextResourceType) {
							// TTML resource found

							GraphicsWidgetTimedTextResource *timelineWidget = dynamic_cast<GraphicsWidgetTimedTextResource*>(p_resource);

							TTMLtimelineSegment ttmlTimelineSegment; // new segment
							TTMLParser *parser = new TTMLParser();

							// new sequence?
							if (ii != last_sequence) sequence_offset = 0;

							ttmlTimelineSegment.in = ((float)timelineWidget->GetFirstVisibleFrame().GetTargetFrame() / 1000);
							ttmlTimelineSegment.out = ((float)timelineWidget->GetLastVisibleFrame().GetTargetFrame() / 1000);
							ttmlTimelineSegment.timeline_in = sequence_offset;
							ttmlTimelineSegment.timeline_out = (ttmlTimelineSegment.out - ttmlTimelineSegment.in) + sequence_offset;

							// is asset alread wrapped in mxf?
							if (p_resource->GetAsset() && p_resource->GetAsset()->HasSourceFiles()) { // no
								// loop source files
								for (int z = 0; z < p_resource->GetAsset()->GetSourceFiles().length(); z++) {
									if (p_resource->InOutChanged) {
										parser->open(p_resource->GetAsset()->GetSourceFiles().at(z), ttmlTimelineSegment, false);
										//p_resource->InOutChanged = false; // don't parse again until next change
									}
								}
							}
							else if(p_resource->GetAsset() && p_resource->InOutChanged){ // yes
								parser->open(p_resource->GetAsset()->GetPath().absoluteFilePath(), ttmlTimelineSegment, true);
								//p_resource->InOutChanged = false; // don't parse again until next change
							}
							else { // ?
								// asset deleted?
							}
							
							parser->~TTMLParser();

							//ttmls.append(ttml_resource);
							sequence_offset += (ttmlTimelineSegment.out - ttmlTimelineSegment.in);

							ttmls->append(ttmlTimelineSegment); // append segment to timeline
							last_sequence = ii;
						}
					}
				}
			}
		}
		// update segment offset
		//segment_offset += p_segment->GetDuration().GetCount();
	}

	emit PlaylistFinished();
	this->thread()->quit();
}